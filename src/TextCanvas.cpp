#include <unordered_map>
#include "TextCanvas.h"

#define FROM_LEFT   1
#define FROM_RIGHT  2
#define FROM_UP     4
#define FROM_DOWN   8


TextCanvas::TextCanvas() {}

void TextCanvas::Resize(size_t height, size_t width) {
    size_t size = (height * width + height) * 24;
    if (size/1024/1024 > 500) {
        std::stringstream ss;
        ss << "Canvas size " << height << " x " << width << " is too large!";
        throw std::runtime_error(ss.str());
    }
    canvas = VVS(height, VS(width, " "));
    overlay = std::vector<std::vector<uint8_t>>(height, std::vector<uint8_t>(width, 0));
}


void TextCanvas::Line(const Pos& a, const Pos& b) {
    auto [ai, aj] = a;
    auto [bi, bj] = b;
    if (ai == bi) {
        auto [x, y] = std::minmax(aj, bj);
        for (size_t j = x; j <= y; j++) {
            // canvas[ai][j] = Line::HORIZON;
            if (j > x && j < y) {
                overlay[ai][j] |= FROM_LEFT | FROM_RIGHT;
            }
        }
        overlay[ai][x] |= FROM_RIGHT;
        overlay[ai][y] |= FROM_LEFT;
    } else if (aj == bj) {
        auto [x, y] = std::minmax(ai, bi);
        for (size_t i = x; i <= y; i++) {
            // canvas[i][aj] = Line::VERTICAL;
            if (i > x && i < y) {
                overlay[i][aj] |= FROM_UP | FROM_DOWN;
            }
        }
        overlay[x][aj] |= FROM_DOWN;
        overlay[y][aj] |= FROM_UP;
    } else {
        DEBUG_OS << ai << ", " << aj << "    " << bi << ", " << bj << "\n";
        throw std::invalid_argument("invalid args");
    }
}

void TextCanvas::Render() {
    auto get_line = [](uint8_t bits) -> std::string {
        int left = !!(bits&FROM_LEFT);
        int right = !!(bits&FROM_RIGHT);
        int up = !!(bits&FROM_UP);
        int down = !!(bits&FROM_DOWN);
        int c = left + right + up + down;
        if (c == 1) {
            if (left || right) return Line::HORIZON;
            else return Line::VERTICAL;
        } else if (c == 2) {
            if (left && right) return Line::HORIZON;
            else if (up && down) return Line::VERTICAL;
            else if (left && up) return Line::RIGHT_BOTTOM;
            else if (right && up) return Line::LEFT_BOTTOM;
            else if (left && down) return Line::RIGHT_TOP;
            else if (right && down) return Line::LEFT_TOP;
        } else if (c == 3) {
            if (!right) return Line::RIGHT_T;
            else if (!left) return Line::LEFT_T;
            else if (!up)  return Line::UP_T;
            else if (!down)  return Line::DOWN_T;
        } else if (c == 4) {
            return Line::CROSS;
        }
        return "?";
    };

    for (size_t i = 0; i < canvas.size(); i++) {
        for (size_t j = 0; j < canvas[i].size(); j++) {
            if (overlay[i][j]) {
                canvas[i][j] = get_line(overlay[i][j]);
            }
        }
    }
}

void TextCanvas::Rect(const Pos& p, size_t height, size_t width,
    TableId style, const std::pair<std::string,std::string>&color) {
    auto [i, j] = p;
    size_t a = i + height - 1;
    size_t b = j + width - 1;
    auto get_line = [style](TableLine line) {
        return get_table_line(style, line);
    };

    auto [start, end] = color;

    for (size_t k = j+1; k < b; k++) {
        canvas[i][k] = get_line(TableLine::TAB_H_LINE);
        canvas[a][k] = get_line(TableLine::TAB_H_LINE);
    }
    for (size_t k = i+1; k < a; k++) {
        canvas[k][j] = start + get_line(TableLine::TAB_V_LINE);
        canvas[k][b] = get_line(TableLine::TAB_V_LINE) + end;
    }

    canvas[i][j] = start + get_line(TableLine::TAB_LEFT_TOP);
    canvas[i][b] = get_line(TableLine::TAB_RIGHT_TOP) + end;
    canvas[a][j] = start + get_line(TableLine::TAB_LEFT_BOTTOM);
    canvas[a][b] = get_line(TableLine::TAB_RIGHT_BOTTOM) + end;
}

std::pair<size_t,size_t> TextCanvas::Size() {
    return {canvas.size(), canvas[0].size()};
}

void TextCanvas::Text(const Pos& p, const std::string& s, Align align) {
    auto [i, j] = p;
    size_t n = visual_len(s);
    overlay[i][j] = 0;
    canvas[i][j] = s;
    if (n <= 1) return;
    if (align == Align::LEFT) {
        for (size_t k = j+1; k <= std::min(j+n-1, canvas[j].size()-1); k++) {
            canvas[i][k] = "";
            overlay[i][k] = 0;
        }
    } else if (align == Align::RIGHT) {
        for (size_t k = j>=n?j-(n-1):0; k <= j-1; k++) {
            canvas[i][k] = "";
            overlay[i][k] = 0;
        }
    } else { // center
        size_t a = j - n/2;
        size_t b = j + n/2 - 1 + (n&1);
        for (size_t k = a; k <= b; k++) {
            canvas[i][k] = "";
            overlay[i][k] = 0;
        }
        canvas[i][j] = s;
    }
}

void TextCanvas::Arrow(const Pos& p, Dir d) {
    std::string s;
    if (d == Dir::Left) {
        s = Line::ARROW_LEFT;
    } else if (d == Dir::Right) {
        s = Line::ARROW_RIGHT;
    } else if (d == Dir::Up) {
        s = Line::ARROW_UP;
    } else if (d == Dir::Down) {
        s = Line::ARROW_DOWN;
    }
    Text(p, s);
}



void TextCanvas::Dump(std::ostream& os) {
    for (size_t i = 0; i < canvas.size(); i++) {
        for (size_t j = 0; j < canvas[i].size(); j++) {
            os << canvas[i][j];
        }
        os << "\n";
    }
}