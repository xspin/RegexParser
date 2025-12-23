#include <sstream>
#include <iostream>
#include <algorithm>
#include "http.h"
#include "utils.h"

// 初始化 Socket 环境（Windows 需初始化 Winsock）
static int socket_init() {
#ifdef _WIN32
    WSADATA wsaData;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", ret);
        return -1;
    }
#endif
    return 0;
}

// 清理 Socket 环境（Windows 需释放）
static void socket_cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// 打印跨平台 Socket 错误信息
static void print_socket_error(const char* msg) {
#ifdef _WIN32
    fprintf(stderr, "code %d\n", GET_ERR());
#else
    perror(msg); // POSIX 用 perror 打印 errno 描述
#endif
}

static bool set_recv_timeout(SOCKET_FD fd, int timeout_ms) {
    if (fd == INVALID_FD) {
        std::cerr << "invalid Socket fd" << std::endl;
        return false;
    }

#ifdef _WIN32
    // Windows：用 DWORD 表示超时时间（毫秒），直接赋值给 TIMEVAL 的 tv_sec/tv_usec 也可
    DWORD timeout = static_cast<DWORD>(timeout_ms);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        std::cerr << "setsockopt failed: " << GET_ERR() << std::endl;
        return false;
    }
#else
    // Linux/macOS：用 struct timeval 表示超时时间
    struct timeval timeout;
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        std::cerr << "setsockopt failed: " << GET_ERR() << std::endl;
        return false;
    }
#endif

    return true;
}

static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    if (start == std::string::npos || end == std::string::npos) {
        return "";
    }
    return str.substr(start, end - start + 1);
}

static void parse_http_request(HttpRequest& req, const std::string& raw_request) {
    size_t i = raw_request.find("\r\n\r\n");
    if (i == std::string::npos) {
        i = raw_request.size();
    }
    std::string headers = raw_request.substr(0, i);
    std::string body = raw_request.substr(i+4);

    std::istringstream iss(raw_request);
    std::string line;

    // 1. 解析请求行（第一行）
    if (!std::getline(iss, line)) {
        throw std::invalid_argument("Empty HTTP request");
    }
    // 去除行尾的 \r（Windows 换行符）
    line = trim(line);
    std::istringstream req_line_iss(line);
    if (!(req_line_iss >> req.method >> req.path >> req.version)) {
        throw std::invalid_argument("Invalid request line: " + line);
    }

    // 2. 解析头信息（直到空行）
    while (std::getline(iss, line)) {
        line = trim(line);
        if (line.empty()) { // 空行，头信息结束
            break;
        }
        // 分割 Key: Value（冒号是分隔符）
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            throw std::invalid_argument("Invalid header: " + line);
        }
        std::string key = trim(line.substr(0, colon_pos));
        std::string value = trim(line.substr(colon_pos + 1));
        // 头信息键不区分大小写，统一转小写（简化处理）
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        req.headers[key] = value;
    }

    // 3. 解析正文（根据 Content-Length）
    if (req.headers.count("content-length")) {
        int content_length = std::stoi(req.headers["content-length"]);
        req.content_length = content_length;
        if (content_length > 0) {
            req.body = body.substr(0, content_length);
        }
    }
}


// 将 HttpResponse 转换为 HTTP 响应字符串
static std::string http_response_to_string(const HttpResponse& res) {
    std::string http_str;

    std::string status_msg = res.status_msg;
    if (status_msg.empty()) {
        switch (res.status_code) {
            case 200: status_msg = "OK"; break;
            case 204: status_msg = "No Content"; break;
            case 404: status_msg = "Not Found"; break;
            case 500: status_msg = "Internal Server Error"; break;
            // ...
            default: status_msg = "Unknown Status";
        }
    }
    http_str += "HTTP/1.1 " + std::to_string(res.status_code) + " " + status_msg + "\r\n";

    std::unordered_map<std::string, std::string> headers = res.headers;
    headers["Content-Length"] = std::to_string(res.body.size());

    if (!res.content_type.empty()) {
        headers["Content-Type"] = res.content_type;
    }

    for (const auto& [k, v] : headers) {
        http_str += k + ": " + v + "\r\n";
    }
    http_str += "\r\n";
    http_str += res.body;

    return http_str;
}

Http::Http(unsigned int port): port(port) {
    client_fd = INVALID_FD;
    fd = INVALID_FD;
}

Http::~Http() {
    if (client_fd > 0) {
        LOG_DEBUG("close fd %lld", client_fd);
        CLOSE_FD(client_fd);
    }
    if (fd > 0) {
        LOG_DEBUG("close fd %lld", fd);
        CLOSE_FD(fd);
        socket_cleanup();
    }
}

int Http::Start() {
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (socket_init() != 0) {
        return -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_FD) {
        print_socket_error("socket");
        socket_cleanup();
        fd = INVALID_FD;
        return -1;
    }

    if (bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        print_socket_error("bind");
        CLOSE_FD(fd);
        socket_cleanup();
        fd = INVALID_FD;
        return -1;
    }

    if (listen(fd, 5) == -1) {
        print_socket_error("listen");
        CLOSE_FD(fd);
        socket_cleanup();
        fd = INVALID_FD;
        return -1;
    }
    printf("Server started on http:://127.0.0.1:%u\n", port);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    while (true) {
        client_fd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == INVALID_FD) {
            print_socket_error("accept");
            continue;
        }
        try {
            HandleConnection(client_fd);
        } catch (const std::exception& e) {
            DEBUG_OS << "error: " << e.what() << "\n";
        }
        CLOSE_FD(client_fd);
        client_fd = INVALID_FD;
    }

    CLOSE_FD(fd);
    socket_cleanup();
    fd = INVALID_FD;
    return 0;

}
#define BUFFER_SIZE 1024

int Http::HandleConnection(SOCKET_FD client_fd) {
    set_recv_timeout(client_fd, 1000);

    bool header = false;
    HttpRequest req;
    char buffer[BUFFER_SIZE];
    std::string data;
    while (true) {
        ssize_t nread = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (nread > 0) {
            if (!header) {
                data.append(buffer, nread);
                if (data.find("\r\n\r\n") != std::string::npos) {
                    // 头部数据接收完整
                    parse_http_request(req, data);
                    header = true;
                }
            } else {
                req.body.append(buffer, nread);
            }
            if (req.body.size() >= req.content_length) {
                break;
            }
        } else if (nread < 0) {
            if (nread != EOF) {
                print_socket_error("recv");
            }
            return nread;
        } else {
            // closed
            return 0;
        }
    }

    try {
        HandleRequest(client_fd, req);
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        RespError(client_fd, 500, e.what());
    }

    return 0;
}


void Http::HandleRequest(SOCKET_FD client_fd, const HttpRequest& req) {
    DEBUG_OS << req.method << " " << req.path << "\n";
    auto it = routes.find(req.path);
    if (it != routes.end()) {
        HttpResponse resp = it->second(req);
        if (resp.status_code == 0) {
            resp.status_code = 200;
        }
        std::string reply = http_response_to_string(resp);
        send(client_fd, reply.c_str(), reply.size(), 0);
    } else {
        DEBUG_OS << "not found route of " << req.path << "\n";
        RespError(client_fd, 404, "<html><head><title>404 Page Not Found</title></head>"
            "<body><h1>404 Page Not Found</h1></body></html>");
    }
}

void Http::RespError(SOCKET_FD client_fd, unsigned int status_code, const std::string& message) {
    DEBUG_OS << "Resp: " << status_code << " " << message << "\n";
    HttpResponse resp;
    resp.status_code = status_code;
    resp.body = message;
    resp.content_type = "text/html";
    std::string reply = http_response_to_string(resp);
    send(client_fd, reply.c_str(), reply.size(), 0);
}

void Http::Route(const std::string& path, RouteFn fn)
{
    routes[path] = fn;
}
