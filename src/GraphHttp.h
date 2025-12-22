#ifndef GRAPHHTTP_H
#define GRAPHHTTP_H

#include "GraphBox.h"
#include "GraphSvg.h"
#include "http.h"
#include "base64.h"

static inline const char* index_html = R"(
<!DOCTYPE html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="shortcut icon" href="data:image/png;base64,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IyI+CiAgPHJkZjpEZXNjcmlwdGlvbiByZGY6YWJvdXQ9IiIKICAgIHhtbG5zOmlwdGNFeHQ9Imh0dHA6Ly9pcHRjLm9yZy9zdGQvSXB0YzR4bXBFeHQvMjAwOC0wMi0yOS8iCiAgICB4bWxuczp4bXBNTT0iaHR0cDovL25zLmFkb2JlLmNvbS94YXAvMS4wL21tLyIKICAgIHhtbG5zOnN0RXZ0PSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvc1R5cGUvUmVzb3VyY2VFdmVudCMiCiAgICB4bWxuczpwbHVzPSJodHRwOi8vbnMudXNlcGx1cy5vcmcvbGRmL3htcC8xLjAvIgogICAgeG1sbnM6R0lNUD0iaHR0cDovL3d3dy5naW1wLm9yZy94bXAvIgogICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICAgeG1sbnM6ZXhpZj0iaHR0cDovL25zLmFkb2JlLmNvbS9leGlmLzEuMC8iCiAgICB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iCiAgIHhtcE1NOkRvY3VtZW50SUQ9ImdpbXA6ZG9jaWQ6Z2ltcDpkYWY1NWZkOC0yNmU0LTQ0NDAtODI3Ny03Mzc1MDZlYWU5NTEiCiAgIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6NGMwMWM3MDgtYmQ4Mi00MjM1LTg0Y2YtYzk0MjUzMzQ1ZjNhIgogICB4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ9InhtcC5kaWQ6MGI2ODkyN2MtMGNlNy00N2RhLTlhMzItMzgxNGIzNzViMTg5IgogICBHSU1QOkFQST0iMi4wIgogICBHSU1QOlBsYXRmb3JtPSJNYWMgT1MiCiAgIEdJTVA6VGltZVN0YW1wPSIxNzY2MzkwMDgzOTYyNzQ3IgogICBHSU1QOlZlcnNpb249IjIuMTAuMTIiCiAgIGRjOkZvcm1hdD0iaW1hZ2UvcG5nIgogICBleGlmOlBpeGVsWERpbWVuc2lvbj0iMTcyIgogICBleGlmOlBpeGVsWURpbWVuc2lvbj0iMTc0IgogICB4bXA6Q3JlYXRvclRvb2w9IkdJTVAgMi4xMCI+CiAgIDxpcHRjRXh0OkxvY2F0aW9uQ3JlYXRlZD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkxvY2F0aW9uQ3JlYXRlZD4KICAgPGlwdGNFeHQ6TG9jYXRpb25TaG93bj4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkxvY2F0aW9uU2hvd24+CiAgIDxpcHRjRXh0OkFydHdvcmtPck9iamVjdD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkFydHdvcmtPck9iamVjdD4KICAgPGlwdGNFeHQ6UmVnaXN0cnlJZD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OlJlZ2lzdHJ5SWQ+CiAgIDx4bXBNTTpIaXN0b3J5PgogICAgPHJkZjpTZXE+CiAgICAgPHJkZjpsaQogICAgICBzdEV2dDphY3Rpb249InNhdmVkIgogICAgICBzdEV2dDpjaGFuZ2VkPSIvIgogICAgICBzdEV2dDppbnN0YW5jZUlEPSJ4bXAuaWlkOjc1M2YyODc0LWYwODctNDU3NC1hMzkwLTVmZDhiMzVlNGNjYyIKICAgICAgc3RFdnQ6c29mdHdhcmVBZ2VudD0iR2ltcCAyLjEwIChNYWMgT1MpIgogICAgICBzdEV2dDp3aGVuPSIyMDI1LTEyLTIyVDE1OjU0OjQzKzA4OjAwIi8+CiAgICA8L3JkZjpTZXE+CiAgIDwveG1wTU06SGlzdG9yeT4KICAgPHBsdXM6SW1hZ2VTdXBwbGllcj4KICAgIDxyZGY6U2VxLz4KICAgPC9wbHVzOkltYWdlU3VwcGxpZXI+CiAgIDxwbHVzOkltYWdlQ3JlYXRvcj4KICAgIDxyZGY6U2VxLz4KICAgPC9wbHVzOkltYWdlQ3JlYXRvcj4KICAgPHBsdXM6Q29weXJpZ2h0T3duZXI+CiAgICA8cmRmOlNlcS8+CiAgIDwvcGx1czpDb3B5cmlnaHRPd25lcj4KICAgPHBsdXM6TGljZW5zb3I+CiAgICA8cmRmOlNlcS8+CiAgIDwvcGx1czpMaWNlbnNvcj4KICAgPGV4aWY6VXNlckNvbW1lbnQ+CiAgICA8cmRmOkFsdD4KICAgICA8cmRmOmxpIHhtbDpsYW5nPSJ4LWRlZmF1bHQiPlNjcmVlbnNob3Q8L3JkZjpsaT4KICAgIDwvcmRmOkFsdD4KICAgPC9leGlmOlVzZXJDb21tZW50PgogIDwvcmRmOkRlc2NyaXB0aW9uPgogPC9yZGY6UkRGPgo8L3g6eG1wbWV0YT4KICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgIAo8P3hwYWNrZXQgZW5kPSJ3Ij8+JBgdLQAAAAZiS0dEAP8A/wD/oL2nkwAAAAlwSFlzAAAWJQAAFiUBSVIk8AAAAAd0SU1FB+kMFgc2K23yexgAABKiSURBVGjebZp5jF31dcc/53fvfdvMvDf75gV77Bnb2NhgmyUOboJNCAkhLKHQbE5ogpRWXaWmVaVWitR/KrWq+k//aJM2kEQhUHAQW0LqstmxCQYbL3gBxjZexh7Pvrz13vs7/ePet5mO9ObNm7ud/Zzv9zwZWjWsIIgIAKoAEH9s+mk+Rz5xjqoitYsVaHhXUKKbGxEQQVVBtfb/6HwAjZ6h0RERECS6TXQAAFcaJFBV6p+bhauLEf8/+tVwjdYfLGDEgXQbFBcRLFYVUWmSUeJ7NF5LbBhEQSQ6qgqiqEqTNKbxblXBRcBIVWMalDHROSKRJYCaPGIwxoFyAeOXIdmG+9g/43x2FyoGyc9iAHFcRARjIm+LEYwx0at6/6qRmuSSZj0RjKoQeS9ylTQq03AiEoeW48QKNFimdzXh7btAFfv5v4YNd0OlADPTUJhGLAT3/T3kliAb78UO/V4knjE14YwxiImEb4yG6mdBInkao2Fo1RqtuuwTFzYIDgLZLpztD2Bf/hGohR2PoVfOwcQ51K/gzF2IrBxbSEM/up9xo3uoRU0SbV8K4mA33Yt74HGkNIMd2YrpHcS++cvYkPXYFhStejpOSlVw67KautAauygWWmIdwuXrMY6H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" />
    <title>Regular Expression Visualization</title>
    <style>
        :root {
            --bg-color: #ffffff;
            --text-color: #333333;
            --btn-bg: #f0f0f0;
            --btn-text: #333333;
            --border-color: #e0e0e0;
            transition: all 0.3s ease;
        }
        :root.dark-mode {
            --bg-color: #1a1a1a;
            --text-color: #cccccc;
            --btn-bg: #333333;
            --btn-text: #f5f5f5;
            --border-color: #444444;
        }
        body {
            background-color: var(--bg-color);
            color: var(--text-color);
            font-size: 15px;
            font-family: monospace, Consolas, Monaco, Menlo;
        }
        .theme-toggle {
            position: fixed;
            top: 20px;
            right: 20px;
            z-index: 999; 
            padding: 5px 10px;
            border: 1px solid var(--border-color);
            border-radius: 6px;
            background-color: var(--btn-bg);
            color: var(--btn-text);
            cursor: pointer;
            font-size: 16px;
            transition: all 0.2s ease;
        }
        .theme-toggle:hover {
            opacity: 0.9;
            background-color: var(--bg-color);
            transform: scale(1.02);
        }

        h1 {
            text-align: center;
            font-size: 24px;
        }

        .input-container {
          margin: 20px;
        }
        .exprinput {
            width: 100%;
            white-space: nowrap;
            overflow-x: auto;
            overflow-y: hidden;
            resize: none;
            padding: 10px 12px;
            border: 1px solid #ccc;
            border-radius: 4px;
            font-size: 14px;
            box-sizing: border-box;
            background-color: var(--bg-color);
            color: var(--text-color);
            margin-top: 1em;
            font-family: ui-monospace,SFMono-Regular,Menlo,Monaco,Consolas,Liberation Mono,Courier New,monospace;
        }

        .exprinput:focus {
            outline: none;
            border-color: #409eff;
            box-shadow: 0 0 0 2px rgba(64, 158, 255, 0.2);
        }

        .exprinput::-webkit-scrollbar {
            height: 6px;
        }
        .exprinput::-webkit-scrollbar-track {
            background: #f1f1f1;
            border-radius: 3px;
        }
        .exprinput::-webkit-scrollbar-thumb {
            background: #ccc;
            border-radius: 3px;
        }
        .exprinput::-webkit-scrollbar-thumb:hover {
            background: #999;
        }

        #diagram {
            white-space: pre;
            margin: 0 20px;
            border: 1px solid gray;
            text-align: center;
            min-width: 2em;
            overflow-x: auto;
            border-radius: 5px;
        }
        #diagram::-webkit-scrollbar {
            height: 10px; 
            background-color: var(--btn-bg);
            border-radius: 4px;
        }
        #diagram::-webkit-scrollbar-thumb {
            background-color: #aaaaaa;
            border-radius: 4px;
            transition: background-color 0.2s ease;
        }
        #diagram svg {
            fill: var(--text-color);
        }
        #diagram svg>#svgbg {
            fill: var(--bg-color);
        }
        #diagram .error {
            color: red;
            text-align: left;
            padding-left: 10px;
        }

        .btn-container {
            display: flex;
            gap: 10px;
            justify-content: right;
            margin-right: 20px;
            margin-bottom: 10px;
        }
        .btn {
            border: 1px solid #ccc;
            border-radius: 8px;
            padding: 6px 5px;
            background-color: var(--btn-bg);
            color: var(--btn-text);
        }
        .btn:hover {
            background: linear-gradient(135deg, #3399ff, #55aaff);
            box-shadow: 0 6px 16px rgba(64, 158, 255, 0.4);
            transform: translateY(-2px);
        }
        .btn:active {
            background: linear-gradient(135deg, #2688eb, #4499ee);
            box-shadow: 0 2px 8px rgba(64, 158, 255, 0.3);
            transform: translateY(0);
        }
        .btn:disabled {
            background: #909399;
            cursor: not-allowed;
            box-shadow: none;
            transform: none;
            opacity: 0.7;
        }
    </style>
</head>
<body>
    <h1>Regular Expression Visualization</h1>
    <button class="theme-toggle" id="themeToggleBtn"></button>

    <div class="input-container"><input type="text" id="exprinput" class="exprinput" placeholder="Input Regular Expression ..."></div>

    <div class="btn-container">
    <button id="randomBtn" class="btn">Random</button>
    <button id="exportBtn" class="btn">Export</button>
    </div>

    <div id="diagram"></div>

<script>
  const exprinput = document.getElementById('exprinput');
  const diagram = document.getElementById('diagram');
  const exportBtn = document.getElementById("exportBtn");
  const randomBtn = document.getElementById("randomBtn");
  exportBtn.disabled = true;

  function base64ToUnicodeStr(base64Str) {
    try {
        const binaryStr = atob(base64Str);
        
        const uint8Array = new Uint8Array(binaryStr.length);
        for (let i = 0; i < binaryStr.length; i++) {
            uint8Array[i] = binaryStr.charCodeAt(i);
        }

        const decoder = new TextDecoder('utf-8');
        return decoder.decode(uint8Array);
    } catch (error) {
        console.error('Base64 decode failed:', error);
        return '';
    }
}

  function debounce(func, delay = 500) {
    let timer = null;
    return function(...args) {
        if (timer) clearTimeout(timer);
        timer = setTimeout(() => {
            func.apply(this, args);
        }, delay);
    };
  }

  async function regexParse(expr, random, callback) {
    exportBtn.disabled = true;
    if (expr === "" && !random) {
      diagram.innerHTML = "";
      if (callback) callback();
      return;
    }

    if (random) {
      exprinput.value = "";
    }

    try {
        const url = random? "/random" : "/regex";
        const response = await fetch(url, {
          method: "POST",
          headers: { 'Content-Type': 'text/plain; charset=utf-8' },
          body: expr
        });
        
        if (!response.ok) {
            throw new Error(`fetch failed: ${response.status}`);
        }

        const data = await response.json();
        if (data.code == 0) {
            diagram.innerHTML = base64ToUnicodeStr(data.data);
            exportBtn.disabled = false;
        } else if (typeof data.msg === 'string' && data.msg !== "") {
            diagram.innerHTML = `<p class="error">${data.msg}</p>`;
        }
      } catch (error) {
        console.error('Error:', error);
        diagram.innerHTML = `<p class="error">Error: ${error}</p>`;
      } finally {
        if (callback) callback()
      }
  }

    exprinput.addEventListener('input', debounce((e) => regexParse(e.target.value, false), 500));
    randomBtn.addEventListener("click", debounce((e) => regexParse("", true), 200));

    const toggleBtn = document.getElementById('themeToggleBtn');
    const root = document.documentElement;
    const savedTheme = localStorage.getItem('theme');
    if (savedTheme === 'dark') {
        root.classList.add('dark-mode');
        toggleBtn.textContent = '☪';
    } else {
        toggleBtn.textContent = '☼';
    }
    toggleBtn.addEventListener('click', () => {
        const isDark = root.classList.toggle('dark-mode');
        toggleBtn.textContent = isDark ? '☪' : '☼';
        localStorage.setItem('theme', isDark ? 'dark' : 'light');
    });


    function getCssVar(varName) {
        const rootStyle = getComputedStyle(document.documentElement);
        return rootStyle.getPropertyValue(varName).trim();
    }

    function svgToPng(svgElement, callback) {
        try {
            const fileName = "regex.png";
            const svgClone = svgElement.cloneNode(true);
            const style = document.createElementNS("http://www.w3.org/2000/svg", "style");
            style.textContent = `#svgbg { fill: ${getCssVar('--bg-color')};} svg {fill: ${getCssVar('--text-color')}}`;
            svgClone.insertBefore(style, svgClone.firstChild);

            const svgData = new XMLSerializer().serializeToString(svgClone);
            const svgBlob = new Blob([svgData], { type: "image/svg+xml;charset=utf-8" });
            const svgUrl = URL.createObjectURL(svgBlob);

            const img = new Image();
            img.crossOrigin = "anonymous";
            img.onload = function() {
                const canvas = document.createElement("canvas");
                canvas.width = svgElement.viewBox.baseVal.width || svgElement.clientWidth;
                canvas.height = svgElement.viewBox.baseVal.height || svgElement.clientHeight;
                const ctx = canvas.getContext("2d");
                // ctx.fillStyle = "#ffffff";
                ctx.fillRect(0, 0, canvas.width, canvas.height);
                ctx.drawImage(img, 0, 0, canvas.width, canvas.height);

                canvas.toBlob(function(blob) {
                    const url = URL.createObjectURL(blob);
                    const a = document.createElement("a");
                    a.href = url;
                    a.download = fileName;
                    document.body.appendChild(a);
                    a.click();
                    document.body.removeChild(a);
                    URL.revokeObjectURL(url);
                    URL.revokeObjectURL(svgUrl);
                    callback();
                }, "image/png");
            };
            img.src = svgUrl;
        } catch (error) {
            console.error("svg export failed:", error);
            alert("Export svg failed!");
            callback();
        }
    }

    exportBtn.addEventListener("click", function() {
        const svgElement = document.getElementById("regexsvg");
        this.disabled = true;
        this.textContent = "Loading...";
        svgToPng(svgElement, ()=>{
            this.disabled = false;
            this.textContent = "Export";
        });
    });

</script>
</body>
</html>

)";

class GraphHttp {
private:
    Http http;

public:
    GraphHttp(int port): http(port) {
        GraphBox::set_encoding(true);
        GraphBox::set_color(true);
    }

    void Start() {

        http.Route("/", [this](const HttpRequest& req){
            return handleIndex(req);
        });

        http.Route("/regex", [this](const HttpRequest& req){
            return handleRegex(req);
        });

        http.Route("/random", [this](const HttpRequest& req){
            return handleRegex(req, true);
        });

        http.Start();
    }

private:
    HttpResponse handleIndex(const HttpRequest& req) {
        HttpResponse resp;
        resp.status_code = 200;
        resp.content_type = "text/html";
        resp.body = index_html;
        return resp;
    }

    HttpResponse handleRegex(const HttpRequest& req, bool random=false) {
        HttpResponse resp;
        resp.status_code = 0;
        resp.content_type = "application/json";

        std::string expr;
        if (random) {
            RegexGenerator g;
            expr = g.generate(30);
            DEBUG_OS << "Random Regex: " << expr << "\n";
        } else {
            expr = utf8_to_uhhhh(req.body);
            DEBUG_OS << "Regex: " << expr << "\n";
        }

        std::string data;
        std::string msg;
        int code = 0;

        try {
            std::unique_ptr<ExprRoot> root(regex_parse(expr));
            if (root) {
                std::unique_ptr<RootBox> box(expr_to_box(root.get()));
                std::string expr_str = root->stringify(true);
                DEBUG_OS << "Parsed: " << expr_str << "\n";
                GraphSvg svg(expr_str, box->get_rows());

                std::stringstream os;
                svg.dump(os);
                data = base64_encode(os.str());
            }
        } catch (const std::exception& e) {
            DEBUG_OS << "Exception: " << e.what() << "\n";
            std::string s = e.what();
            for (char c : s) {
                if (c == '\n') {
                    msg += "</br>";
                } else if (c == '\\') {
                    msg += "&#x2F;";
                } else if (c == '"') {
                    msg += "&quot;";
                } else if (c == '&') {
                    msg += "&amp;";
                } else if (c == '<') {
                    msg += "&lt;";
                } else if (c == '>') {
                    msg += "&gt;";
                } else {
                    msg += c;
                }
            }
            code = 1;
        }

        std::stringstream ss;
        ss << "{"
            << "\"data\":\"" << data << "\","
            << "\"msg\":\"" << msg << "\","
            << "\"code\":" << code
            << "}";

        resp.body = ss.str();

        return resp;
    }
};


#endif // GRAPHHTTP_H