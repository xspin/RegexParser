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
        body {  margin: 20px;}
        pre { background: #f4f4f4; padding: 10px; border-radius: 5px; }
        .diagram {
            line-height: 1.3;
            vertical-align: middle;
            font-size: 15px;
            font-family: monospace, Consolas, Monaco, Menlo;
            letter-spacing: 0; white-space: pre;}
        .diagram p { margin: 0 10px; }
        .diagram #expr { 
            text-decoration: none;
            padding: 4px 4px;
            border: gray 1px solid;
            border-radius: 3px;
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
    <div class="diagram">{{DIAGRAM}}</div>
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
        if (i+1 < n && s[i] == '\033' && s[i+1] == '[') {
            size_t j = i + 2;
            while (j < s.size() && s[j] != 'm') j++;

            std::string code = s.substr(i, j - i + 1);
            if (code == UNDERLINE) {
                underline = true;
                i = j + 1;
                continue;
            } else if (code == NC) {
                ss << "</span>";
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
        auto graph = color_replace(text_graph);
        auto regex = "<p>Regular Expression: <a id=\"expr\">" 
            + color_replace(expr) + "</a></p>";

        html = html_tpl;
        std::string key = "{{DIAGRAM}}";
        size_t pos = html.find(key);
        if (pos != std::string::npos) {
            html.replace(pos, key.size(), regex + "\n" + graph);
        }
    }
};

#endif
