#ifndef HTTP_H
#define HTTP_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <functional>

// 跨平台适配宏
#ifdef _WIN32
    // Windows 平台
    #include <winsock2.h>
    #include <ws2tcpip.h>
    // #pragma comment(lib, "ws2_32.lib") // 链接 Winsock 库
    typedef SOCKET SOCKET_FD;
    #define INVALID_FD INVALID_SOCKET
    #define CLOSE_FD(fd) closesocket(fd)
    #define GET_ERR() WSAGetLastError()
#else
    // Linux/macOS 平台
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <errno.h>
    typedef long long SOCKET_FD;
    #define INVALID_FD -1
    #define CLOSE_FD(fd) close(fd)
    #define GET_ERR() errno
#endif

#include <unordered_map>

// HTTP 请求解析结果
struct HttpRequest {
    std::string method;       // 请求方法：GET/POST/PUT/DELETE 等
    std::string path;         // 请求路径：/index.html
    std::string version;      // HTTP 版本：HTTP/1.1
    std::unordered_map<std::string, std::string> headers; // 头信息
    size_t content_length = 0;
    std::string body;         // 请求正文
};

// HTTP 响应解析结果
struct HttpResponse {
    unsigned status_code = 0;      // 状态码：200/404/500 等
    std::string status_msg;   // 状态描述：OK/Not Found
    std::unordered_map<std::string, std::string> headers; // 头信息
    std::string content_type;
    std::string body;         // 响应正文
};

class Http {
    using RouteFn = std::function<HttpResponse(const HttpRequest& req)>;

    unsigned int port;
    SOCKET_FD fd;
    SOCKET_FD client_fd;
    std::unordered_map<std::string,RouteFn> routes;

public:
    Http(unsigned int port);
    ~Http();
    void Route(const std::string& path, RouteFn fn);
    int Start();

private:
    int HandleConnection(SOCKET_FD client_fd);
    void HandleRequest(SOCKET_FD client_fd, const HttpRequest& req);
    void RespError(SOCKET_FD client_fd, unsigned int status_code, const std::string& message);
};

#endif // HTTP_H