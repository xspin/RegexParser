#ifndef __GRAPHBOX_H__
#define __GRAPHBOX_H__

#include <string>
#include <vector>
#include <stack>
#include "utils.h"
#include "Parser.h"
#include "unicode.h"

using Rows = std::vector<std::string>;

enum class Dir {
    Center,
    Left,
    Right,
    Up,
    Down,
    Horizon,
    Vertical
};

enum class TableId {
    DEFAULT = 0,
    ROUND,
    DASHED,
    BOLD,
    BOLD_DASHED,
    DOUBLE,
    EMPTY
};

enum TableLine {
    TAB_LEFT_TOP = 0,
    TAB_H_LINE,
    TAB_RIGHT_TOP,
    TAB_V_LINE,
    TAB_LEFT_BOTTOM,
    TAB_RIGHT_BOTTOM,
    TAB_RIGHT_T,
    TAB_LEFT_T,
    TAB_UP_T,
    TAB_DOWN_T,
    TAB_MAX
};

#define SPACE "☐"

namespace Line {
    const std::string HORIZON = "─";
    const std::string HORIZON_DASHED = "╴";
    const std::string VERTICAL = "│"; 
    const std::string VERTICAL_DASHED = "╷"; 
    const std::string LEFT_TOP = "┌";
    const std::string LEFT_BOTTOM = "└";
    const std::string RIGHT_TOP = "┐";
    const std::string RIGHT_BOTTOM = "┘";
    const std::string CROSS = "┼";
    const std::string RIGHT_T = "┤";
    const std::string LEFT_T = "├";
    const std::string UP_T =  "┬";
    const std::string DOWN_T =  "┴";
    const std::string START = "●";
    const std::string TERMINAL = "◎";
    const std::string LU_CORNER = "╭";
    const std::string LD_CORNER = "╰";
    const std::string RU_CORNER = "╮";
    const std::string RD_CORNER = "╯";
    const std::string ARROW_UP = "↑";
    const std::string ARROW_DOWN = "↓";
    const std::string ARROW_LEFT = "←";
    const std::string ARROW_RIGHT = "→";
};

using Align = Utils::Align;

std::string get_table_line(TableId id, TableLine t);

/**
 * \brief get the visual width of a string
 */
static inline size_t visual_len(const std::string& s) {
    return visual_width(s);
}

static inline std::string visual_str_pad(const std::string& s, size_t n,
    Align align, const std::string& p=" ") {
    size_t w = visual_len(s);
    // if (n < w) DEBUG_OS << "length exceeds\n";
    if (n <= w) return s;
    return Utils::str_pad(s, n-w, align, p);
}

using Block = std::vector<std::vector<std::string>>;

Block build_block(size_t width, size_t height, const std::string& tag="", TableId id=TableId::DEFAULT);

Block block_concat(const Block& a, const Block& b, size_t spaces=0, Dir d=Dir::Horizon);

static inline std::string get_quantifier(int min, int max, bool lazy) {
    std::string arrow = lazy? "⇢" : "→";
    // std::string arrow = "->";
    std::string a = std::to_string(min);
    std::string b = max < INF? std::to_string(max) : "∞";
    std::string c = min > 0 ? "↻" : "↺";
    std::stringstream ss;
    if (min != max) {
        ss << c << a << arrow << b;
    } else {
        ss << c << a;
    }
    return ss.str();
}

struct Quant {
    std::string str;
    bool lazy;

    Quant(const Quantifier* q) {
        str = get_quantifier(q->min, q->max, q->lazy);
        lazy = q->lazy;
    }
};

struct Span {
    size_t left = 0;
    size_t right = 0;
    size_t top = 0;
    size_t bottom = 0;

    Span& operator+=(Span& rhs) {
        this->left += rhs.left;
        this->right += rhs.right;
        this->top += rhs.top;
        this->bottom += rhs.bottom;
        return *this;
    }
    Span operator+(Span& rhs) {
        Span res;
        res += rhs;
        return res;
    }
};

std::pair<Rows,Span> box_table(TableId id, const Rows& lines,
    const std::string& top="");

std::pair<Rows,Span> box_bottom(
    const Rows& lines,
    const std::string& tag, bool dashed=false);

std::pair<Rows,Span> box_expand(
    const Rows& lines, size_t width,
    const std::string& link=Line::HORIZON, 
    Align align=Align::CENTER);

static inline std::pair<Rows,Span> box_ext(TableId id,
    const Rows& lines,
    const std::string& tag, const Quant* q=nullptr) {
    auto [res, span] = box_table(id, lines, tag);
    if (q) {
        auto t = box_bottom(res, q->str, q->lazy);
        res = t.first;
        span += t.second;
    }
    return {res, span};
}

static inline
std::pair<Rows,Span> box_normal(const Rows& lines, const Quant* q=nullptr) {
    return box_ext(TableId::DEFAULT, lines, "", q);
}

static inline
std::pair<Rows,Span> box_class(const Rows& lines, const std::string& tag, const Quant* q=nullptr) {
    return box_ext(TableId::BOLD, lines, tag, q);
}

static inline
std::pair<Rows,Span> box_negclass(const Rows& lines, const std::string& tag, const Quant* q=nullptr) {
    return box_ext(TableId::BOLD_DASHED, lines, tag, q);
}

static inline
std::pair<Rows,Span> box_group(const Rows& lines, const std::string& tag, const Quant* q=nullptr) {
    return box_ext(TableId::DASHED, lines, tag, q);
}

static inline
std::pair<Rows,Span> box_empty(const Rows& lines, const Quant* q=nullptr) {
    return box_ext(TableId::EMPTY, lines, "", q);
}

static inline
std::pair<Rows,Span> box_special(const Rows& lines, const std::string& tag="", const Quant* q=nullptr) {
    return box_ext(TableId::DOUBLE, lines, tag, q);
}

enum class BoxType {
    NORMAL,
    RANGE,
    CLASS,
    GROUP,
    ASSERTION,
    BRANCH,
    EMPTY,
    LINK,
    ANCHOR,
    QUANTIFIER,
    ESCAPED,
    ROOT,
};

class GraphBox {
public:
    GraphBox(BoxType t): type(t) {
    }

    virtual ~GraphBox() {
        for (auto p : child) {
            delete p;
        }
    }

    BoxType get_type() {
        return type;
    }

    std::pair<size_t,size_t> get_position() {
        return {x, y};
    }

    const Rows& get_rows() {
        return rows;
    }

    const std::string get_raw() {
        return raw;
    }

    size_t get_mid() {
        return rows.size()>>1;
    }

    size_t get_height() {
        return rows.size();
    }

    size_t get_width() {
        return rows.empty()? 0 : visual_len(rows[0]);
    }

    ExprNode* get_expr() {
        return expr;
    }

    void set_expr(ExprNode* expr) {
        this->expr = expr;
    }


    static void set_encoding(bool utf8) {
        GraphBox::utf8_encoding = utf8;
    }

    static void set_color(bool color) {
        GraphBox::color = color;
        reset_color();
    }

    std::string pack_color(const std::string& s) {
        if (!GraphBox::color) return s;
        if (s.empty()) {
            iter_color();
            return s;
        }
        return iter_color_pack(s);
    }

    void dump(std::ostream& os) {
        for (const auto& row : rows) {
            os << row << "\n";
        }
    }

    void set_quantifier(const Quantifier* q) {
        quant = std::make_unique<Quant>(q);
        quant->str = pack_color(quant->str);
    }

    void set_position(size_t x, size_t y) {
        this->x = x;
        this->y = y;
    }

    const std::vector<GraphBox*>* get_child() {
        return &child;
    }

    virtual void render() =0;
    // virtual void layout() =0;

protected:
    BoxType type;
    Rows rows;
    std::string raw;
    ExprNode* expr = nullptr;
    std::vector<GraphBox*> child;
    std::string top;
    std::unique_ptr<Quant> quant;
    // Span span;
    size_t x = 0;
    size_t y = 0;
    static bool color;
    static bool utf8_encoding;
};

static inline std::string unescape(const std::string& s) {
    std::string res;
    size_t n = s.size();
    for (size_t i = 0; i < n; ++i) {
        if (s[i] == '\\' && i+1 < n) {
            res += s[i+1];
            ++i;
        } else if (s[i] == ' ') {
            res += SPACE;
        } else {
            res += s[i];
        }
    }
    return res;
}

class NormalBox: public GraphBox {
public:
    NormalBox(const std::string& s): GraphBox(BoxType::NORMAL) {
        this->raw = unescape(s);
    }

    void render() {
        auto t = box_normal({this->raw}, quant.get());
        this->rows = t.first;
        // this->span = t.second;
    }

    // void layout() {
    //     x += span.left;
    //     y += span.top;
    // }

};


class RangeBox: public GraphBox {
public:
    RangeBox(const std::string& s): GraphBox(BoxType::RANGE) {
        this->raw = s;
    }

    void render() {
        raw = pack_color(raw);
        auto t = box_normal({raw}, quant.get());
        this->rows = t.first;
        // this->span = t.second;
    }

    // void layout() {
    //     x += span.left;
    //     y += span.top;
    // }
};

class ClassBox: public GraphBox {
public:
    ClassBox(GraphBox* sub, bool negative=false): GraphBox(BoxType::CLASS), negative(negative)  {
        child.push_back(sub);

        if (negative) {
            top = "None of";
        } else {
            top = "One of";
        }
    }

    void render() {
        Rows lines;

        auto append_line = [&lines](GraphBox* box) {
            std::string s = box->get_raw();
            if (s.empty()) return;
            if (GraphBox::color && box->get_type() != BoxType::NORMAL) {
                s = UNDERLINE + s + NC;
            }
            lines.push_back(s);
        };

        // align color iterator
        auto tp = pack_color(top);

        child.front()->render();

        // child may just be a NormalBox
        append_line(child.front());

        auto link_box = child.front();
        auto items = link_box->get_child(); 
        for (auto sub : *items) {
            append_line(sub);
        }
        assert(!lines.empty());

        std::pair<Rows,Span> t;
        if (negative) {
            t = box_negclass(lines, tp, quant.get());
        } else {
            t = box_class(lines, tp, quant.get());
        }
        this->rows = t.first;
        // this->span = t.second;
    }

    /*
    void layout() {
        auto dx = this->span.left;
        auto dy = this->span.top;
        auto link_box = child.front();
        link_box->set_position(x+dx, y+dy);
        for (auto sub : *link_box->get_child()) {
            sub->set_position(x+dx, y+dy);
            dy += 1;
            // sub->layout(); // raw data no need layout
        }
    }
    */

private:
    bool negative;
};

class GroupBox: public GraphBox {
public:
    GroupBox(GraphBox* sub, int gid=0): GraphBox(BoxType::GROUP) {
        top = gid? "Group #" + std::to_string(gid) : "";
        child.push_back(sub);
    }

    void render() {
        auto tp = pack_color(top);
        child.front()->render();
        const Rows& rows = child.front()->get_rows();
        auto t = box_group(rows, tp, quant.get());
        this->rows = t.first;
        // this->span = t.second;
    }

    // void layout() {
    //     size_t dx = this->span.left;
    //     size_t dy = this->span.top;
    //     child.front()->set_position(x+dx, y+dy);
    //     child.front()->layout();
    // }
};

class AssertionBox: public GraphBox {
public:
    AssertionBox(GraphBox* sub, const std::string& tag): GraphBox(BoxType::ASSERTION) {
        child.push_back(sub);
        this->tag = tag;
    }

    void render() {
        if (tag == "?=") {
            top = "Lookahead";
        } else if (tag == "?!") {
            top = "!Lookahead";
        } else if (tag == "?<=") {
            top = "Lookbehind";
        } else if (tag == "?<!") {
            top = "!Lookbehind";
        } else {
            top = tag;
            DEBUG_OS << "Not implement for " << tag << "\n";
        }
        auto tp = pack_color(top);
        child.front()->render();
        const Rows& rows = child.front()->get_rows();
        auto t = box_group(rows, tp);
        this->rows = t.first;
        // this->span = t.second;
    }

    // void layout() {
    //     size_t dx = this->span.left;
    //     size_t dy = this->span.top;
    //     child.front()->set_position(x+dx, y+dy);
    //     child.front()->layout();
    // }

private:
    std::string tag;
};

class EmptyBox: public GraphBox {
public:
    EmptyBox(): GraphBox(BoxType::EMPTY) {
    }
    void render() {
        rows.push_back(Line::HORIZON);
        // this->span = Span{0,0,0,0};
    }
    // void layout() {
        // do nothing
    // }
};

class BranchBox: public GraphBox {
    std::vector<Span> spans;

public:
    BranchBox(std::vector<GraphBox*> branches): GraphBox(BoxType::BRANCH) {
        child = branches;
    }

    void render() {
        iter_color(); // align color iterator
        size_t width = 0;
        size_t height = 0;
        for (auto branch : child) {
            branch->render();
            width = std::max(width, branch->get_width());
            height += branch->get_height();
        }
        std::string vline = Line::VERTICAL;

        size_t first = child.front()->get_height() / 2;
        size_t last = height - (child.back()->get_height()+1) / 2;

        size_t i = 0;
        Rows rows;

        size_t mid = height / 2;
        for (auto branch : child) {
            size_t branch_mid = i + branch->get_mid();
            auto [tmp, sp] = box_expand(branch->get_rows(), width);
            spans.push_back(sp);
            for (const std::string& row : tmp) {
                if (i < first || i > last) {
                    rows.push_back(" " + row + " ");
                } else if (i == first) {
                    if (i == mid) rows.push_back(Line::UP_T + row + Line::UP_T);
                    else rows.push_back(Line::LEFT_TOP + row + Line::RIGHT_TOP);
                } else if (i == last) {
                    if (i == mid) rows.push_back(Line::DOWN_T + row + Line::DOWN_T);
                    else rows.push_back(Line::LEFT_BOTTOM + row + Line::RIGHT_BOTTOM);
                } else if (i == branch_mid) {
                    if (i == mid) rows.push_back(Line::CROSS + row + Line::CROSS);
                    else rows.push_back(Line::LEFT_T + row + Line::RIGHT_T);
                } else if (i == mid) {
                    rows.push_back(Line::RIGHT_T + row + Line::LEFT_T);
                } else {
                    rows.push_back(vline + row + vline);
                }
                i++;
            }
        }
        // keep height odd
        if (is_even(height)) {
            rows.push_back(std::string(width+2, ' '));
            // this->span.bottom += 1;
        }
        auto t = box_expand(rows, width+4);
        this->rows = t.first;
        // this->span += t.second;
    }

    /*
    void layout() {
        size_t dx = this->span.left + 2;
        size_t dy = this->span.top;
        for (size_t i = 0; i < child.size(); i++) {
            auto sub = child[i];
            sub->set_position(x+dx + spans[i].left, y+dy + spans[i].top);
            dy += sub->get_height();
            sub->layout();
        }
    }*/
};


class QuantBox: public GraphBox {
public:
    QuantBox(GraphBox* prev): GraphBox(BoxType::QUANTIFIER) {
        child.push_back(prev);
    }

    void render() {
        ExprNode* expr = get_expr();
        assert(expr && expr->isQuantifier());

        Quantifier* q = static_cast<Quantifier*>(expr);

        child.front()->set_quantifier(q);
        child.front()->render();
        this->rows = child.front()->get_rows();
        // this->span = {0};
    }

    // void layout() {
    //     child.front()->set_position(x + span.left, y + span.top);
    //     child.front()->layout();
    // }
};

class LinkBox: public GraphBox {
    // std::vector<size_t> span_tops;

public:
    LinkBox(std::vector<GraphBox*> items): GraphBox(BoxType::LINK) {
        child = items;
    }

    void render() {
        size_t height = 0;
        for (auto item : child) {
            item->render();
            height = std::max(height, item->get_height());
        }

        size_t mid = height / 2;
        this->rows = Rows(height, "");

        for (auto item : child) {
            link(item, mid);
        }
    }

    void link(GraphBox* box, size_t mid) {
        size_t i = mid - box->get_height()/2;
        // span_tops.push_back(i);

        std::string spaces(box->get_width(), ' ');
        for (size_t j=0; j<i; j++) {
            this->rows[j] += spaces;
        }

        for (const std::string& row : box->get_rows()) {
            this->rows[i++] += row;
        }

        for (size_t j=i; j<this->rows.size(); j++) {
            this->rows[j] += spaces;
        }
    }

    // void layout() {
    //     size_t dx = span.left;
    //     for (size_t i = 0; i < child.size(); i++) {
    //         auto sub = child[i];
    //         sub->set_position(x+dx, y + span.top + span_tops[i]);
    //         dx += sub->get_width();
    //         sub->layout();
    //     }
    // }
};

class AnchorBox: public GraphBox {

public:
    AnchorBox(const std::string& symbol): GraphBox(BoxType::ANCHOR) {
        raw = symbol;
    }

    void render() {
        std::string symbol = raw;
        std::string s;
        if (symbol == "^") {
            s = "Start";
        } else if (symbol == "$") {
            s = "End";
        } else if (symbol == "\\b") {
            s = "Word Boundary";
        } else if (symbol == "\\B") {
            s = "Non-Word Boundary";
        } else if (symbol == ".") {
            s = "Any";
        } else {
            DEBUG_OS << "No implement for anchor " << symbol << "\n";
            s = symbol;
        }
        auto t = box_special({pack_color(s)}, symbol, quant.get());
        rows = t.first;
        // span = t.second;
    }
    // void layout() {
    //     x += span.left;
    //     y += span.top;
    // }
};

class EscapedBox: public GraphBox {
/*
        \d  [0-9]
        \D  [^0-9]
        \w  [A-Za-z0-9_]	
        \W  [^A-Za-z0-9_]	
        \s  [ \t\n\r\f\v]
        \S  [^ \t\n\r\f\v]
        \n  换行符（Line Feed）
        \r  回车符（Carriage Return）
        \t  制表符（Tab）
        \f  换页符（Form Feed）
        \v  垂直制表符（Vertical Tab）
        \0  空字符（Null）
        \xHH
        \uHHHH
*/
public:
    EscapedBox(const std::string symbol): GraphBox(BoxType::ESCAPED), symbol(symbol) {
        std::string s;

        if (symbol == "\\d") {
            s = "Digit";
        } else if (symbol == "\\D") {
            s = "Non-Digit";
        } else if (symbol == "\\w") {
            s = "Word";
        } else if (symbol == "\\W") {
            s = "Non-Word";
        } else if (symbol == "\\s") {
            s = "Space";
        } else if (symbol == "\\S") {
            s = "Non-Space";
        } else if (symbol.size() == 4 && symbol.substr(0,2) == "\\x") {
            s = "0x" + symbol.substr(2);
            std::transform(s.begin()+2, s.end(), s.begin()+2, ::toupper);
        } else if (symbol.size() == 6 && symbol.substr(0,2) == "\\u") {
            if (utf8_encoding) {
                s = uhhhh_to_utf8(symbol);
            } else {
                s = "U+" + symbol.substr(2);
                std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            }
        } else {
            // todo ...
            s = symbol;
        }
        raw = s;
    }

    void render() {
        raw = pack_color(raw);
        auto t = box_special({raw}, symbol, quant.get());
        rows = t.first;
        // span = t.second;
    }

    // void layout() {
    //     x += span.left;
    //     y += span.top;
    // }

private:
    std::string symbol;
};


class RootBox: public GraphBox {
public:
    RootBox(GraphBox* box): GraphBox(BoxType::ROOT) {
        child.push_back(box);
    }

    void render() {
        auto p = child.front();
        p->render();

        size_t w = p->get_width();
        w += 2;
        auto t = box_expand(p->get_rows(), w, Line::HORIZON);
        Rows tmp = t.first;
        // this->span = t.second;
        // this->span.left += 2;
        // this->span.right += 2;

        w += 1;
        t = box_expand(tmp, w, Line::START, Align::RIGHT);
        tmp = t.first;

        w += 1;
        t = box_expand(tmp, w, Line::TERMINAL, Align::LEFT);
        this->rows = t.first;
    }

    // void layout() {
    //     x = 0;
    //     y = 0;
    //     auto p = child.front();
    //     p->set_position(x + span.left, y + span.top);
    //     p->layout();
    // }
};

std::unique_ptr<RootBox> expr_to_box(ExprNode* root);

static inline void travel_box(GraphBox* box, std::function<void(GraphBox*)> func) {
    std::stack<GraphBox*> stk;
    stk.push(box);

    while (!stk.empty()) {
        GraphBox* box = stk.top();
        stk.pop();
        func(box);
        for (auto sub : *box->get_child()) {
            stk.push(sub);
        }
    }
}

#endif // __GRAPHBOX_H__