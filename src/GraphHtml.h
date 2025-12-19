#ifndef GRAPHHTML_H
#define GRAPHHTML_H
#include <string>
#include "Parser.h"
#include "unicode.h"

static const char* html_tpl = R"(
<!DOCTYPE html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
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
            --text-color: #bbbbbb;
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
            bottom: 20px;
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
            transform: scale(1.02);
        }
        .header {
            position: fixed;
            top: 0;
            left: 0;
            padding: 20px 0;
            background-color: var(--bg-color);
            display: flex;
            flex-wrap: nowrap;
            z-index: 99;
            width: 100%;
        }
        .header #label {
            flex: 0;
            padding: 4px 0px;
            margin-right: 5px;
            margin-left: 10px;
            width: fit-content;
            white-space: nowrap;
        }
        .header #expr { 
            text-decoration: none;
            padding: 4px 4px;
            border: gray 1px solid;
            border-radius: 3px;
            overflow-y: hidden;
            overflow-x: auto; 
            white-space: nowrap;
            min-width: 200px;
            flex: 1;
            margin-right: 20px;
            max-width: fit-content;
        }
        #expr::-webkit-scrollbar {
            height: 8px; 
            background-color: var(--btn-bg);
            border-radius: 4px;
        }
        #expr::-webkit-scrollbar-thumb {
            background-color: #aaaaaa;
            border-radius: 4px;
            transition: background-color 0.2s ease;
        }
        .diagram {
            line-height: 1.3;
            vertical-align: middle;
            letter-spacing: 0;
            white-space: pre;
            margin-top: 80px;
        }
        .diagram .double-width {
            display: inline-block;
            width: 2ch;
            text-align: center;
        }
        .diagram .triple-width {
            display: inline-block;
            width: 3ch;
            text-align: center;
        }
    </style>
</head>
<body>
    <button class="theme-toggle" id="themeToggleBtn"></button>
    <div class="header">{{HEADER}}</div>
    <div class="diagram">{{DIAGRAM}}</div>
<script>
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
</script>
</body>
</html>
)";


static inline std::string color_replace(const std::string& s) {
    std::stringstream ss;
    size_t i = 0;
    size_t n = s.size();
    bool underline = false;
    bool color_open = false;
    while (i < s.size()) {
        if (i+1 < n && s[i] == ESC_PRE && s[i+1] == '[') {
            size_t j = i + 2;
            while (j < s.size() && s[j] != 'm') j++;

            std::string code = s.substr(i, j - i + 1);
            if (code == UNDERLINE) {
                underline = true;
                i = j + 1;
                continue;
            } else if (code == NC) {
                if (color_open) ss << "</span>";
                color_open = false;
                i = j + 1;
                continue;
            }
            if (color_open) {
                ss << "</span>";
                color_open = false;
            }

            std::string color = esc_code_color(code);
            if (underline) {
                ss << "<span style=\"color:" << color << "; text-decoration:underline;\">";
                underline = false;
                color_open = true;
            } else if (!color.empty()) {
                ss << "<span style=\"color:" << color << ";\">";
                color_open = true;
            }
            i = j + 1;
        } else {
            if (s[i] == '<') {
                ss << "&lt;";
            } else if (s[i] == '>') {
                ss << "&gt;";
            } else if (s[i] == '&') {
                ss << "&amp;";
            } else {
                size_t k = utf8_next(s.data()+i);
                if (k == 1) {
                    ss << s[i];
                } else {
                    const std::string t = s.substr(i, k);
                    size_t w = visual_width(t);
                    if (w == 1) {
                        ss << t;
                    } else if (w == 2) {
                        ss << "<span class=\"double-width\">" << t << "</span>";
                    } else {
                        ss << "<span class=\"triple-width\">" << t << "</span>";
                    }
                    i += (k - 1);
                }
            }
            i++;
        }
    }
    return ss.str();
}

class GraphHtml {
    std::string expr;
    std::string text_graph;
    std::string html;

public:
    GraphHtml(const std::string& expr, const std::string& graph)
    : expr(expr), text_graph(graph) {
        render();
    }

    void dump(std::ostream& os) {
        os << html;
    }

private:
    void render() {
        html = html_tpl;

        auto header = "<div id=\"label\">Regular Expression:</div><div id=\"expr\">" 
            + color_replace(expr) + "</div>";

        replace("{{HEADER}}", header);

        auto graph = color_replace(text_graph);
        replace("{{DIAGRAM}}", graph);
    }

    void replace(const std::string& key, const std::string& value) {
        size_t pos = html.find(key);
        if (pos != std::string::npos) {
            html.replace(pos, key.size(), value);
        }
    }
};

#endif
