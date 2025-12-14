#include "TextCanvas.h"

#define SPACES      0   // default value
#define FROM_LEFT   1
#define FROM_RIGHT  2
#define FROM_UP     4
#define FROM_DOWN   8
#define LINED       (FROM_LEFT|FROM_RIGHT|FROM_UP|FROM_DOWN)
#define FILLED      16
#define EMPTY       32

static std::string get_line(uint8_t bits) {
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
    DEBUG_OS << "some thing maybe wrong\n";
    return bits? " " : "";
};

TextCanvas::TextCanvas(size_t height, size_t width): height(height),width(width) {
    assert(height > 0 && width > 0);
}

void TextCanvas::Resize(size_t height, size_t width) {
    assert(height > 0 && width > 0);

    LOG_DEBUG("Change Canvas size %lu x %lu\n", height, width);

    this->height = height;
    this->width = width;
    canvas.clear();
    overlay.clear();
    size = 0;
}

void TextCanvas::Set(const Pos& p, const std::string& s) {
    auto [i, j] = p;

    if (i >= height || j >= width) {
        throw std::out_of_range("invalid postion");
    }
    if (s.empty()) {
        overlay[i][j] = EMPTY;
        size++;
    } else {
        overlay[i][j] = FILLED;
        canvas[i][j] = s;
        size += 1;
    }
    if (size > 1024*1024*20) {
        throw std::overflow_error("TextCanvas " + std::to_string(size) + " elements, too large!");
    }
}

static inline uint8_t get_mask(const MMU& overlay, size_t i, size_t j) {
    uint8_t mask = 0; 
    auto row_it = overlay.find(i);
    if (row_it != overlay.end()) {
        auto it = row_it->second.find(j);
        mask = (it == row_it->second.end())? 0 : it->second;
    }
    return mask;
}

std::string TextCanvas::Get(const Pos& p) {
    auto [i, j] = p;
    if (i >= height || j >= width) {
        return "";
    }

    uint8_t mask = get_mask(overlay, i, j);

    std::string s;
    if (mask == SPACES) {
        s = " ";
    } else if (mask == EMPTY) {
        s = "";
    } else if (mask == FILLED) {
        s = canvas[i][j];
    } else {
        s = get_line(mask);
    }
    return s;
}

void TextCanvas::Line(const Pos& a, const Pos& b) {
    auto [ai, aj] = a;
    auto [bi, bj] = b;
    if (ai == bi) {
        auto [x, y] = std::minmax(aj, bj);
        for (size_t j = x; j <= y; j++) {
            if (j > x && j < y) {
                overlay[ai][j] |= FROM_LEFT | FROM_RIGHT;
                size++;
            }
        }
        overlay[ai][x] |= FROM_RIGHT;
        overlay[ai][y] |= FROM_LEFT;
        size += 2;
    } else if (aj == bj) {
        auto [x, y] = std::minmax(ai, bi);
        for (size_t i = x; i <= y; i++) {
            if (i > x && i < y) {
                overlay[i][aj] |= FROM_UP | FROM_DOWN;
                size++;
            }
        }
        overlay[x][aj] |= FROM_DOWN;
        overlay[y][aj] |= FROM_UP;
        size += 2;
    } else {
        DEBUG_OS << ai << ", " << aj << "    " << bi << ", " << bj << "\n";
        throw std::invalid_argument("invalid args");
    }
}


void TextCanvas::Rect(const Pos& p, size_t height, size_t width,
    TableId style, const std::pair<std::string,std::string>&color) {
    auto [i, j] = p;
    size_t a = i + height - 1;
    size_t b = j + width - 1;
    auto tab_line = [style](TableLine line) {
        return get_table_line(style, line);
    };

    auto [start, end] = color;

    for (size_t k = j+1; k < b; k++) {
        Set({i,k}, tab_line(TableLine::TAB_H_LINE));
        Set({a,k}, tab_line(TableLine::TAB_H_LINE));
    }
    for (size_t k = i+1; k < a; k++) {
        Set({k,j}, start + tab_line(TableLine::TAB_V_LINE));
        Set({k,b}, tab_line(TableLine::TAB_V_LINE) + end);
    }
    Set({i, j}, start + tab_line(TableLine::TAB_LEFT_TOP));
    Set({i, b}, tab_line(TableLine::TAB_RIGHT_TOP) + end);
    Set({a, j}, start + tab_line(TableLine::TAB_LEFT_BOTTOM));
    Set({a, b}, tab_line(TableLine::TAB_RIGHT_BOTTOM) + end);
}

std::pair<size_t,size_t> TextCanvas::Size() {
    return {height, width};
}

void TextCanvas::Text(const Pos& p, const std::string& s, Align align) {
    auto [i, j] = p;
    size_t n = visual_len(s);
    Set({i,j}, s);
    if (n <= 1) return;
    if (align == Align::LEFT) {
        for (size_t k = j+1; k <= std::min(j+n-1, width-1); k++) {
            Set({i,k}, "");
        }
    } else if (align == Align::RIGHT) {
        for (size_t k = j>=n?j-(n-1):0; k <= j-1; k++) {
            Set({i,k}, "");
        }
    } else { // center
        size_t a = j - n/2;
        size_t b = j + n/2 - 1 + (n&1);
        for (size_t k = a; k <= b; k++) {
            Set({i,k}, "");
        }
        Set({i,j}, s);
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
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            std::string s = Get({i,j});
            os << s;
        }
        os << "\n";
    }
}