#include <numeric>
#include <iostream>
#include "GraphBox.h"
#include <unordered_map>

static const std::vector<std::vector<std::string>> s_tables = {
    {"┌", "─", "┐", "│", "└", "┘", "┤", "├", "┬", "┴"}, // Default / Normal
    {"╭", "─", "╮", "│", "╰", "╯", "┼", "┼", "┬", "┴"}, // Round
    {"╭", "╴", "╮", "╷", "╰", "╯", "┼", "┼", "┬", "┴"}, // Dashed / Group
    {"┏", "━", "┓", "┃", "┗", "┛", "┫", "┣", "┳", "┻"}, // Bold / Class
    {"┏", "╸", "┓", "┃", "┗", "┛", "┫", "┣", "┳", "┻"}, // BoldDashed / Neg-Class
    {"╔", "═", "╗", "║", "╚", "╝", "╣", "╠", "╦", "╩"}, // Double
    {" ", " ", " ", " ", " ", " ", " ", " ", " ", " "}, // Empty
};

static std::unordered_map<std::string,std::string> s_map;

std::string get_table_line(TableId id, TableLine t) {
    return s_tables[static_cast<int>(id)][t];
}

std::string rotate(const std::string& s) {
    if (s.size() <= 1) return s;
    if (s_map.empty()) {
        for (size_t i=0; i<s_tables.size()-1; i++) {
            const auto& tb = s_tables[i];
            s_map[tb[TAB_LEFT_TOP]] = tb[TAB_RIGHT_TOP];
            s_map[tb[TAB_RIGHT_TOP]] = tb[TAB_RIGHT_BOTTOM];
            s_map[tb[TAB_RIGHT_BOTTOM]] = tb[TAB_LEFT_BOTTOM];
            s_map[tb[TAB_LEFT_BOTTOM]] = tb[TAB_LEFT_TOP];
            s_map[tb[TAB_H_LINE]] = tb[TAB_V_LINE];
            s_map[tb[TAB_V_LINE]] = tb[TAB_H_LINE];
            s_map[tb[TAB_RIGHT_T]] = tb[TAB_DOWN_T];
            s_map[tb[TAB_DOWN_T]] = tb[TAB_LEFT_T];
            s_map[tb[TAB_LEFT_T]] = tb[TAB_UP_T];
            s_map[tb[TAB_UP_T]] = tb[TAB_RIGHT_T];
        }
    }
    auto it = s_map.find(s);
    if (it != s_map.end()) return it->second;
    return s;
}

/*
    make each line same length
*/
static Rows rows_pad(const Rows& lines) {
    size_t width = 0;
    size_t min_width = INT_MAX;
    for (const auto& s : lines) {
        width =  std::max(width, visual_len(s));
        min_width =  std::min(min_width, visual_len(s));
    }
    if (min_width == width) return lines;

    Rows res;
    for (auto& line : lines) {
        res.push_back(visual_str_pad(line, width, Align::LEFT));
    }
    return res;
}


/*
    Expand each line to left to right

    Make sure all lines same length
*/
std::pair<Rows,Span> box_expand(const Rows& lines, size_t width, const std::string& link, Align align) {
    assert(!lines.empty());

    Span span;

    if (width <= visual_len(lines.front())) {
        return {lines, span};
    }

    size_t w = width - visual_len(lines.front());

    size_t mid = lines.size() / 2;
    size_t lw = w / 2;
    size_t rw = w - lw;
    std::string lspaces = std::string(lw, ' ');
    std::string rspaces = std::string(rw, ' ');

    if (align == Align::LEFT) {
        span.right = lw + rw;
    } else if (align == Align::RIGHT) {
        span.left = lw + rw;
    } else {
        span.left = lw;
        span.right = rw;
    }

    Rows res;
    for (size_t i=0; i<lines.size(); i++) {
        if (i == mid && !link.empty()) {
            if (align == Align::LEFT) {
                res.push_back(lines[i] + Utils::str_repeat(link, w));
            } else if (align == Align::RIGHT) {
                res.push_back(Utils::str_repeat(link, w) + lines[i]);
            } else {
                res.push_back(Utils::str_repeat(link, lw) + lines[i] + Utils::str_repeat(link, rw));
            }
        } else {
            if (align == Align::LEFT) {
                res.push_back(lines[i] + lspaces + rspaces);
            } else if (align == Align::RIGHT) {
                res.push_back(lspaces + rspaces + lines[i]);
            } else {
                res.push_back(lspaces + lines[i] + rspaces);
            }
        }
    }
    return {res, span};
}


/* Make sure each line same length */
std::pair<Rows,Span> box_bottom(const Rows& lines, const std::string& tag, bool dashed) {
    assert(!lines.empty());
    Rows res;
    Span span;
    size_t height = lines.size();
    size_t mid = height / 2;
    size_t width = visual_len(lines.front());

    Rows rows;
    if (width < visual_len(tag)) {
        width = visual_len(tag);
        auto t = box_expand(lines, width, Line::HORIZON);
        rows = t.first;
        span = t.second;
    } else {
        rows = lines;
    }

    std::string vline = dashed? Line::VERTICAL_DASHED : Line::VERTICAL;
    std::string hline = Line::HORIZON;

    span.left += 2;
    span.right += 2;
    span.bottom += 1; 

    if (is_even(height+1)) {
        res.push_back(std::string(width+4, ' '));
    }

    size_t i = 0;
    for (; i < mid; i++) {
        res.push_back("  " + rows[i] + "  ");
    }

    res.push_back(Line::HORIZON + Line::UP_T + rows[i++] + Line::UP_T + Line::HORIZON);

    for (; i < height; i++) {
        res.push_back(" " + vline + rows[i] + vline + " ");
    }

    res.push_back(
        " " + Line::LEFT_BOTTOM 
        + visual_str_pad(tag, width, Align::CENTER, hline) 
        + Line::RIGHT_BOTTOM + " ");

    return {res,span};
}

std::pair<Rows,Span> box_table(TableId id, const Rows& lines, const std::string& top) {
    assert(!lines.empty());
    Rows res;
    Span span;
    const auto& tab = s_tables[static_cast<int>(id)];

    // pad each line to same length
    Rows rows = rows_pad(lines);

    size_t width = visual_len(rows.front());

    size_t len = visual_len(top);

    std::pair<Rows,Span> t;
    if (width < len) {
        if (id == TableId::DASHED) { // for Group
            t = box_expand(rows, len, Line::HORIZON);
        } else {
            t = box_expand(rows, len, "");
        }
        rows = t.first;
        span = t.second;
    }

    width = std::max(width, len) + 2;

    size_t margin = 1;
    size_t height = rows.size() + 2;

    std::string padding = Utils::str_repeat(" ", margin);
    std::string link = Utils::str_repeat(Line::HORIZON, margin);
    // std::string rlink = Utils::str_repeat(Line::HORIZON, margin-1) + ">";
    span.left += margin + 1;
    span.right += margin + 1;

    size_t mid = height / 2;

    std::string h_line = Utils::str_repeat(tab[TAB_H_LINE], width-2);

    span.top += 1;
    res.push_back(padding + tab[TAB_LEFT_TOP] 
        + visual_str_pad(top, width-2, Align::CENTER, tab[TAB_H_LINE]) 
        + tab[TAB_RIGHT_TOP] + padding);

    for (const auto& line : rows) {
        size_t i = res.size();
        if (i == mid) {
            res.push_back(link + tab[TAB_RIGHT_T]
                + visual_str_pad(line, width-2, Align::CENTER)
                + tab[TAB_LEFT_T] + link);
        } else {
            res.push_back(padding + tab[TAB_V_LINE]
                + visual_str_pad(line, width-2, Align::CENTER)
                + tab[TAB_V_LINE] + padding);
        }
        
    }
    res.push_back(padding + tab[TAB_LEFT_BOTTOM] + h_line + tab[TAB_RIGHT_BOTTOM] + padding);
    span.bottom += 1;

    return {res, span};
}


Block build_block(size_t width, size_t height, const std::string& tag, TableId id) {
    assert(width>=2 && height>=2);
    const auto& tab = s_tables[static_cast<int>(id)];

    Block res(height, std::vector<std::string>(width, " "));
    for (size_t j=1; j < width-1; j++) {
        res.front()[j] = tab[TAB_H_LINE];
        res.back()[j] = tab[TAB_H_LINE];
    }
    for (size_t i=1; i < height-1; i++) {
        res[i].front() = tab[TAB_V_LINE];
        res[i].back() = tab[TAB_V_LINE];
    }
    res[0].front() = tab[TAB_LEFT_TOP];
    res[0].back() = tab[TAB_RIGHT_TOP];
    res.back().front() = tab[TAB_LEFT_BOTTOM];
    res.back().back() = tab[TAB_RIGHT_BOTTOM];

    if (!tag.empty()) {
        size_t empty = (width-2)*(height-2);
        auto r = utf8_split(tag);
        size_t x = r.size() >= empty ? 0 : (empty-r.size())/2;
        size_t k = 0;

        for (size_t i = 1; i < height-1; i++) {
            for (size_t j = 1; j < width-1; j++) {
                if (x > 0) {
                    x--;
                } else if (k < r.size()) {
                    res[i][j] = tag.substr(r[k].first, r[k].second);
                    k++;
                }
            }
        }
    }

    return res;
}

Block block_concat(const Block& a, const Block& b, size_t spaces, Dir d) {
    if (a.empty()) return b;
    if (b.empty()) return a;

    auto append_spaces = [](std::vector<std::string>& vec, size_t n) {
        while (n--) vec.push_back(" ");
    };

    auto append_vec = [](std::vector<std::string>& lhs, const std::vector<std::string>& rhs) {
        for (const std::string& s : rhs) {
            lhs.push_back(s);
        }
    };

    Block rows;
    for (size_t i = 0; i < a.size(); i++) {
        rows.push_back(a[i]);
    }
    if (d == Dir::Vertical) {
        if (a.front().size() < b.front().size()) {
            size_t d = b.front().size() - a.front().size();
            for (size_t i = 0; i < a.size(); i++) {
                append_spaces(rows[i], d);
            }
        }
        for (size_t i = 0; i < spaces; i++) {
            rows.push_back(std::vector<std::string>(rows.front().size(), " "));
        }
        for (size_t i = 0; i < b.size(); i++) {
            rows.push_back(b[i]);
            append_spaces(rows.back(), rows.front().size()-b[i].size());
        }
    } else { // Horizon
        std::string s = std::string(spaces, ' ');
        size_t i = 0;
        for (; i<std::min(a.size(), b.size()); i++) {
            append_spaces(rows[i], spaces);
            append_vec(rows[i], b[i]);
        }
        for (; i<b.size(); i++) {
            rows.push_back({});
            append_spaces(rows.back(), a.back().size() + spaces);
            append_vec(rows.back(), b[i]);
        }
    }
    return rows;
}

std::unique_ptr<RootBox> expr_to_box(ExprNode* expr) {
    assert(expr);

    std::stack<std::pair<int,GraphBox*>> stk;

    int level = 0;
    expr->travel([&level](ExprNode* node) {
        ++level;
    },  [&](ExprNode* node) {
        GraphBox* p = nullptr;
        GraphBox* t = stk.empty()? nullptr : stk.top().second;

        if (node == nullptr) {
            p = new EmptyBox();
        } else if (node->isType(ExprType::T_CLASS)) {
            auto cls = static_cast<Class*>(node);
            assert(t);
            p = new ClassBox(t, cls->negative);
            stk.pop();
        } else if (node->isType(ExprType::T_GROUP)) {
            auto group = static_cast<Group*>(node);
            assert(t);
            p = new GroupBox(t, group->id, group->name);
            stk.pop();
        } else if (node->isType(ExprType::T_QUANTIFIER)) {
            assert(t);
            p = new QuantBox(t);
            stk.pop();
        } else if (node->isType(ExprType::T_SEQUENCE)) {
            std::vector<GraphBox*> tmp;
            while (!stk.empty() && level+1 == stk.top().first) {
                tmp.push_back(stk.top().second);
                stk.pop();
            }
            p = new LinkBox(tmp);
        } else if (node->isType(ExprType::T_OR)) {
            std::vector<GraphBox*> tmp;
            while (!stk.empty() && level+1 == stk.top().first) {
                tmp.push_back(stk.top().second);
                stk.pop();
            }
            p = new BranchBox(tmp);
        } else if (node->isType(ExprType::T_LOOKAHEAD)) {
            assert(t);
            auto look = static_cast<Lookahead*>(node);
            p = new AssertionBox(t, look->negative? "?!" : "?=");
            stk.pop();
        } else if (node->isType(ExprType::T_LOOKBEHIND)) {
            assert(t);
            auto look = static_cast<Lookbehind*>(node);
            p = new AssertionBox(t, look->negative? "?<!" : "?<=");
            stk.pop();
        } else if (node->isType(ExprType::T_ANCHOR)) {
            p = new AnchorBox(node->str(false));
        } else if (node->isType(ExprType::T_ANY)) {
            p = new AnchorBox(".");
        } else if (node->isType(ExprType::T_ESCAPED)) {
            p = new EscapedBox(node->str());
        } else if (node->isType(ExprType::T_RANGE)) {
            auto range = static_cast<Range*>(node);
            p = new RangeBox(range->start, range->end);
        } else if (node->isType(ExprType::T_BACKREF)) {
            auto ref = static_cast<Backref*>(node);
            p = new BackrefBox(ref->str(false), ref->id, ref->name);
        } else {
            // T_LITERAL,
            p = new NormalBox(node->str());
        }

        if (p) {
            p->set_expr(node);
            stk.emplace(level, p);
            // DEBUG_OS << "push: " << level << " " << p << " " << (node? node->str():"null") << "\n";
        }
        --level;
    }, true);

    assert(!stk.empty());

    RootBox* root = new RootBox(stk.top().second);

    stk.pop();
    while (!stk.empty()) {
        auto t = stk.top().second;
        DEBUG_OS << "pop: " << stk.top().first << " " << t
            << (t->get_expr()?t->get_expr()->str() : "")
            << " Maybe something wrong!\n";
        stk.pop();
    }

    reset_color();

    root->render();
    // root->layout();
    return std::unique_ptr<RootBox>(root);
};

bool GraphBox::color = false;
bool GraphBox::utf8_encoding = false;
