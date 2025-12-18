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

static inline void parse_line(std::vector<std::string>&res, const std::string& line)
{
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
    size_t graph_width;

public:
    GraphSvg(const std::string& expr, const std::vector<std::string>& graph): expr(expr) {
        for (const std::string &row : graph) {
            rows.push_back({});
            parse_line(rows.back(), row);
        }
        graph_width = visual_width(graph[0]);
        render();
    }

    void dump(std::ostream& os) {
        os << svg;
    }

private:
    std::pair<size_t,size_t> render_expr(std::ostream& os) {
        size_t limit = 100;
        size_t expr_y = FONT_SIZE*2;
        size_t w = visual_width(expr);
        std::vector<std::string> line;
        std::string label = "Regular Expression: ";
        parse_line(line, label + expr);

        std::vector<std::vector<std::string>> lines;
        if (w <= limit || w <= graph_width) {
            lines.push_back(line);
        } else {
            size_t n = line.size();
            size_t i = 0;;
            while (i < n) {
                size_t j = i + limit;
                if (j >= n) {
                    lines.emplace_back(line.begin() + i, line.end());
                    break;
                }
                while (j > i && line[j][0] != ESC_PRE) j--;
                if (j <= i) j = i + limit;
                lines.emplace_back(line.begin() + i, line.begin() + j);
                i = j;
            }
        }

        os << "<text x=" << strwraperr(0)
           << " y=" << strwraperr(expr_y)
           << " font-size=" << strwraperr(FONT_SIZE)
           << " font-family=\"Consolas, Monaco, 'Courier New', monospace\""
           << ">\n";


        size_t expr_x = (label.size()+1) * FONT_WIDTH;
        size_t max_x = 0;
        for (size_t i = 0; i < lines.size(); i++) {
            size_t x = i == 0? FONT_WIDTH : expr_x;
            size_t t = render_line(os, lines[i], expr_y, x);
            max_x = std::max(max_x, t);
            expr_y += FONT_SIZE;
        }

        os << "</text>\n";

        os << "<rect x=" << strwraperr(expr_x-2)
           << " y=" << strwraperr(FONT_SIZE)
           << " width=" << strwraperr(max_x - expr_x + 4)
           << " height=" << strwraperr(lines.size()*FONT_SIZE+4)
           << " fill=\"none\""
           << " stroke-width=\"1\""
           << " stroke=\"gray\""
           << "/>\n";

        return {expr_y + FONT_SIZE, max_x};
    }

    void render() {
        std::stringstream expr_ss;
        auto [graph_y, expr_w] = render_expr(expr_ss);

        std::stringstream graph_ss;
        size_t max_x = expr_w;
        size_t y = graph_y;
        for (auto& line : rows) {
            size_t x = render_line(graph_ss, line, y, FONT_WIDTH);
            y += FONT_SIZE;
            max_x = std::max(max_x, x);
        }

        size_t height = y;
        size_t width = max_x + FONT_WIDTH;

        std::stringstream ss;
        ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           << "<svg width=" << strwraperr(width)
           << " height=" << strwraperr(height)
           << " xmlns=" << "\"http://www.w3.org/2000/svg\"" << ">\n";

        ss << R"(<rect x="0" y="0" width="100%" height="100%" fill="#fafafa" />)";

        ss << expr_ss.str();

        ss << "<text x=" << strwraperr(0)
           << " y=" << strwraperr(graph_y)
           << " font-size=" << strwraperr(FONT_SIZE)
           << " font-family=\"Consolas, Monaco, 'Courier New', monospace\""
           << " letter-spacing=\"0\""
           << " text-anchor=\"start\""
           << " white-space=\"pre\">\n";

        ss << graph_ss.str();

        ss << "</text>" << "</svg>\n";
        svg = ss.str();
    }

    size_t render_line(std::ostream& os, const std::vector<std::string>& line, size_t y, size_t x = 0) {
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
        return x;
    }

    void tspan(std::ostream& os, const std::string& content
        , const std::string& xs, size_t y, const std::string& fill="", bool underline=false) {
        os << "<tspan " << "y=" << strwraperr(y);
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


