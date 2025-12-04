#include <string>
#include <vector>
#include <stack>
#include "utils.h"
#include "parser.h"
#include "unicode.h"

using Rows = std::vector<std::string>;

// static const std::string space = "⏡";

enum class TableId {
    DEFAULT = 0,
    DASHED,
    BOLD,
    BOLD_DASHED,
    DOUBLE,
    EMPTY
};

namespace Line {
    const std::string HORIZON = "─";
    const std::string HORIZON_DASHED = "╴";
    const std::string VERTICAL = "│"; 
    const std::string VERTICAL_DASHED = "┆"; 
    const std::string LEFT_TOP = "┌";
    const std::string LEFT_BOTTOM = "└";
    const std::string RIGHT_TOP = "┐";
    const std::string RIGHT_BOTTOM = "┘";
    const std::string CROSS = "┼";
    const std::string RIGHT_T = "┤";
    const std::string LEFT_T = "├";
    const std::string UP_T =  "┬";
    const std::string DOWN_T =  "┴";
    const std::string TERMINAL = "●";
};

static inline size_t visual_len(const std::string& s) {
    return visual_width(s);
}

static inline std::string visual_str_pad(const std::string& s, size_t n,
    Utils::Align align, const std::string& p=" ") {
    size_t w = visual_len(s);
    if (n < w) DEBUG_OS << "length exceeds\n";
    if (n <= w) return s;
    return Utils::str_pad(s, n-w, align, p);
}


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

Rows box_table(TableId id, const Rows& lines,
    const std::string& top="");

Rows box_bottom(
    const Rows& lines,
    const std::string& tag, bool dashed=false);

Rows box_expand(
    const Rows& lines, size_t width,
    const std::string& link=Line::HORIZON);

static inline Rows box_ext(TableId id,
    const Rows& lines,
    const std::string& tag, const Quant* q=nullptr) {
    Rows res = box_table(id, lines, tag);
    if (q) {
        res = box_bottom(res, q->str, q->lazy);
    }
    return res;
}

static inline
Rows box_normal(const Rows& lines, const Quant* q=nullptr) {
    return box_ext(TableId::DEFAULT, lines, "", q);
}

static inline
Rows box_class(const Rows& lines, const std::string& tag, const Quant* q=nullptr) {
    return box_ext(TableId::BOLD, lines, tag, q);
}
static inline
Rows box_negclass(const Rows& lines, const std::string& tag, const Quant* q=nullptr) {
    return box_ext(TableId::BOLD_DASHED, lines, tag, q);
}

static inline
Rows box_group(const Rows& lines, const std::string& tag, const Quant* q=nullptr) {
    return box_ext(TableId::DASHED, lines, tag, q);
}

static inline
Rows box_empty(const Rows& lines, const Quant* q=nullptr) {
    return box_ext(TableId::EMPTY, lines, "", q);
}

static inline
Rows box_special(const Rows& lines, const std::string& tag="", const Quant* q=nullptr) {
    return box_ext(TableId::DOUBLE, lines, tag, q);
}

class GraphBox {
public:
    GraphBox(): expr(nullptr) {
    }

    virtual ~GraphBox() {
        for (auto p : child) {
            delete p;
        }
    }

    const Rows& get_rows() {
        return rows;
    }

    const Rows& get_raw() {
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
        return iter_color_pack(s);
    }

    void dump(std::ostream& os) {
        for (const auto& row : rows) {
            os << row << "\n";
        }
    }

    // void dump_vertical(std::ostream& os) {
    //     size_t w = get_width();
    //     size_t h = get_height();
    //     std::vector<const char*> bytes;
    //     for (const std::string& line : rows) {
    //         bytes.push_back(line.data());
    //     }

    //     for (size_t i=0; i<w; i++) {
    //         for (int r = h-1; r >= 0; r--) {
    //             size_t k = utf8_next(bytes[r]);
    //             std::string s(bytes[r], k);
    //             bytes[r] += k;
    //             os << rotate(s);
    //         }
    //         os << "\n";
    //     }
    // }

    void set_quantifier(const Quantifier* q) {
        quant = std::make_unique<Quant>(q);
    }

    const std::vector<GraphBox*>* get_child() {
        return &child;
    }

    virtual void render() =0;

protected:
    Rows rows;
    Rows raw;
    ExprNode* expr;
    std::vector<GraphBox*> child;
    std::string top;
    std::unique_ptr<Quant> quant;
    static bool color;
    static bool utf8_encoding;
};

class NormalBox: public GraphBox {
public:
    NormalBox(const std::string& s) {
        if (s.size() == 2 && s[0] == '\\') {
            this->raw = {pack_color(s)};
        } else {
            this->raw = {s};
        }
    }
    void render() {
        this->rows = box_normal(raw, quant.get());
    }
};


class RangeBox: public GraphBox {
public:
    RangeBox(const std::string& s) {
        this->raw = {pack_color(s)};
    }

    void render() {
        this->rows = box_normal(raw, quant.get());
    }
};

class ClassBox: public GraphBox {
public:
    ClassBox(GraphBox* sub, bool negative=false): negative(negative) {
        child.push_back(sub);

        if (negative) {
            top = "None of";
        } else {
            top = "One of";
        }
    }

    void render() {
        Rows lines;
        // child may just be a NormalBox
        for (const std::string& s : child.front()->get_raw()) {
            lines.push_back(s);
        }

        auto link_box = child.front();
        auto items = link_box->get_child(); 
        for (auto sub : *items) {
            sub->render();
            for (const std::string& s : sub->get_raw()) {
                lines.push_back(s);
            }
        }
        assert(!lines.empty());

        if (negative) {
            this->rows = box_negclass(lines, top, quant.get());
        } else {
            this->rows = box_class(lines, top, quant.get());
        }
    }
private:
    bool negative;
};

class GroupBox: public GraphBox {
public:
    GroupBox(GraphBox* sub, int gid=0) {
        top = gid? "Group #" + std::to_string(gid) : "";
        child.push_back(sub);
    }

    void render() {
        child.front()->render();
        const Rows& rows = child.front()->get_rows();
        this->rows = box_group(rows, top, quant.get());
    }
};

class AssertionBox: public GraphBox {
public:
    AssertionBox(GraphBox* sub, const std::string& tag) {
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
        child.front()->render();
        const Rows& rows = child.front()->get_rows();
        this->rows = box_group(rows, top);
    }

private:
    std::string tag;
};

class EmptyBox: public GraphBox {
public:
    EmptyBox() {
    }
    void render() {
        rows.push_back(Line::HORIZON);
    }
};

class BranchBox: public GraphBox {
public:
    BranchBox(std::vector<GraphBox*> branches) {
        child = branches;
    }

    void render() {
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
            Rows tmp = box_expand(branch->get_rows(), width);
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
        }
        this->rows = box_expand(rows, width+4);
    }
};


class QuantBox: public GraphBox {
public:
    QuantBox(GraphBox* prev) {
        child.push_back(prev);
    }

    void render() {
        ExprNode* expr = get_expr();
        assert(expr && expr->isQuantifier());

        Quantifier* q = static_cast<Quantifier*>(expr);

        child.front()->set_quantifier(q);
        child.front()->render();
        this->rows = child.front()->get_rows();
    }
};

class LinkBox: public GraphBox {
public:
    LinkBox(std::vector<GraphBox*> items) {
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
};

class AnchorBox: public GraphBox {

public:
    AnchorBox(const std::string& symbol) {
        raw = {symbol};
    }

    void render() {
        std::string symbol = raw.front();
        std::string s;
        if (symbol == "^") {
            s = "Start";
        } else if (symbol == "$") {
            s = "End";
        } else if (symbol == "\\b") {
            s = "Word Bound";
        } else if (symbol == "\\B") {
            s = "Non-Word Bound";
        } else if (symbol == ".") {
            s = "Any";
        } else {
            DEBUG_OS << "No implement for anchor " << symbol << "\n";
            s = symbol;
        }
        rows = box_special({pack_color(s)}, symbol, quant.get());
    }
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
    EscapedBox(const std::string symbol): symbol(symbol) {
        init();
    }

    void init() {
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
            std::transform(s.begin()+2, s.end(), s.begin()+2, [](char c){return std::toupper(c);});
        } else if (symbol.size() == 6 && symbol.substr(0,2) == "\\u") {
            if (utf8_encoding) {
                s = uhhhh_to_utf8(symbol);
            } else {
                s = "U+" + symbol.substr(2);
                std::transform(s.begin(), s.end(), s.begin(), [](char c){return std::toupper(c);});
            }
        } else {
            // todo ...
            s = symbol;
        }
        raw = {pack_color(s)};
    }

    void render() {
        rows = box_special(raw, symbol, quant.get());
    }

private:
    std::string symbol;
};


class RootBox: public GraphBox {
public:
    RootBox(GraphBox* box) {
        child.push_back(box);
    }

    void render() {
        auto p = child.front();
        p->render();

        Rows tmp = box_expand(p->get_rows(), p->get_width()+2, Line::HORIZON);
        this->rows = box_expand(tmp, p->get_width()+4, Line::TERMINAL);
    }
};

std::unique_ptr<RootBox> expr_to_box(ExprNode* root);
