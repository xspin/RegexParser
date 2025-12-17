#ifndef GRAPHSVG_H
#define GRAPHSVG_H

#include "Parser.h"

#define FONT_SIZE 14
#define FONT_WIDTH 8

template<typename T>
static inline std::string strwraperr(const T& t) {
    return "\"" + std::to_string(t) + "\"";
}
static inline std::string strwraperr(const std::string& t) {
    return "\"" + t + "\"";
}

static inline void parse_line(std::vector<std::string>&res, const std::string& line) {

    auto vec = utf8_split(line);
    for (size_t i = 0; i < vec.size(); i++) {
        auto [j, k] = vec[i];
        // \033[00m
        if (k == 1 && line[j] == ESC_PRE && i + 4 < vec.size()) {
            auto t = vec[i+4];
            if (t.second == 1 && line[t.first] == 'm') {
                res.push_back(line.substr(j, 5));
                i += 4;
                continue;
            }
        }
        res.push_back(line.substr(j, k));
    }
};

class GraphSvg {
    std::string expr;
    std::vector<std::vector<std::string>> rows;
    std::string svg;
    size_t expr_y = FONT_SIZE;
    size_t graph_y = FONT_SIZE*3;

public:
    GraphSvg(const std::string& expr, const std::vector<std::string>& graph): expr(expr) {
        for (const std::string &row : graph) {
            rows.push_back({});
            parse_line(rows.back(), row);
        }

        render();
    }

    void dump(std::ostream& os) {
        os << svg;
    }

private:
    void render_expr(std::ostream& os) {
        std::vector<std::string> line;
        parse_line(line, "Regular Expression: " + expr);

        os << "<text x=" << strwraperr(0)
           << " y=" << strwraperr(expr_y)
           << " font-size=" << strwraperr(FONT_SIZE)
           << " font-family=\"Consolas, Monaco, 'Courier New', monospace\""
           << ">\n";

        render_line(os, line, expr_y);

        os << "</text>";
    }

    void render() {
        size_t height = FONT_SIZE * (rows.size() + 3);
        size_t width = FONT_WIDTH * (std::max(rows[0].size(), visual_width(expr)+22));

        std::stringstream ss;
        ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<svg width=" << strwraperr(width)
           << " height=" << strwraperr(height)
           << " xmlns=" << "\"http://www.w3.org/2000/svg\"" << ">\n";

        render_expr(ss);

        ss << "\n<text x=" << strwraperr(0)
           << " y=" << strwraperr(graph_y)
           << " font-size=" << strwraperr(FONT_SIZE)
           << " font-family=\"Consolas, Monaco, 'Courier New', monospace\""
           << " letter-spacing=\"0\""
           << " text-anchor=\"start\""
           << " white-space=\"pre\">\n";

        size_t y = graph_y;
        for (auto& line : rows) {
            render_line(ss, line, y);
            y += FONT_SIZE;
        }

        ss << "</text>" << "</svg>\n";
        svg = ss.str();
    }

    void render_line(std::ostream& os, const std::vector<std::string>& line, size_t y) {
        auto escape = [](std::string s) {
            std::string res;
            for (char c : s) {
                switch (c) {
                case '<':
                    res += "&lt;"; break;
                case '>':
                    res += "&gt;"; break;
                case '&':
                    res += "&amp;"; break;
                default:
                    res += c;
                }
            }
            return res;
        };

        size_t n = line.size();
        size_t x = 0;
        size_t i = 0;
        size_t x_space = FONT_WIDTH;
        auto is_esc = [](const std::string& s) {
            return s[0] == ESC_PRE;
        };

        while (i < n) {
            size_t k = i;
            while (k < n && line[k] == " ") {
                k++;
            }
            if (k > i) {
                x += (k-i) * x_space;
                i = k;
                if (i >= n) break;
            }

            std::vector<std::string> ws;

            std::string color;
            bool underline = false;
            size_t j = i;
            std::string text;

            if (is_esc(line[j])) {
                while (j < n && line[j] != NC) {
                    if (is_esc(line[j])) {
                        if (line[j] == UNDERLINE) {
                            underline = true;
                        } else {
                            if (!color.empty()) {
                                j--;
                                break;
                            }
                            color = esc_code_color(line[j]);
                        }
                    } else {
                        ws.push_back(std::to_string(x));
                        x += visual_width(line[j]) * x_space;
                        text += escape(line[j]);
                    }
                    j++;
                }
                j++; // NC
            } else {
                while (j < n && line[j] != " " && !is_esc(line[j])) {
                    ws.push_back(std::to_string(x));
                    x += visual_width(line[j]) * x_space;
                    text += escape(line[j]);
                    j++;
                }
            }

            auto xs =  Utils::concat(ws, " ");
            tspan(os, text, xs, y, color, underline);

            i = j;
        }
    }

    void tspan(std::ostream& os, const std::string& content
        , const std::string& xs, size_t y, const std::string& fill="", bool underline=false) {
        os << "<tspan " << " y=" << strwraperr(y);
        os << " x=" << strwraperr(xs);
        if (!fill.empty()) {
           os << " fill=" << strwraperr(fill);
        }
        if (underline) {
            os << " text-decoration=\"underline\"";
        }
        os << ">" << content << "</tspan>\n";
    }
};

#endif


