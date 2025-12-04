#include "parser.h"

int ExprNode::indent = 0;

static std::vector<std::string> colors = {GREEN, BLUE, YELLOW, PURPLE, RED, CYAN};
static int color_idx = 0;

void reset_color() {
    color_idx = 0;
}

std::string iter_color() {
    color_idx %= colors.size();
    return colors[color_idx++];
}


static inline std::pair<int,int> parse_quantifier(char ch) {
    switch (ch) {
        case '?': return {0, 1};
        case '*': return {0, INF};
        case '+': return {1, INF};
    }
    return {-1, -1};
}

static inline std::pair<int,int> parse_quantifier(const std::string& s) {
    assert (s.front() == '{' && s.back() == '}');

    std::string t = s.substr(1, s.size()-2);
    size_t i = t.find(',');
    int a, b;

    if (i == std::string::npos) {
        a = stoi(t);
        return {a, a};
    }

    a = stoi(t.substr(0, i));
    b = i == t.size()-1 ? INF : stoi(t.substr(i+1));

    return {a, b};
}

static inline std::pair<std::string,std::string> parse_range(const std::string& s) {
    size_t i = s.substr(1, s.size()-2).find('-') + 1;
    std::string a = s.substr(0, i);
    std::string b = s.substr(i+1);
    return {a, b};
}


std::string ExprNode::prefix(int indent) {
    if (this->indent > 0) {
        return "\n" + std::string(indent, ' ');
    } else {
        return "";
    }
}

std::string ExprNode::format(int indent, bool color) {
    reset_color();
    this->indent = indent;
    if (indent > 0) {
        size_t pos = 0;
        size_t width = 0;
        std::string f = this->fmt(0, color) + "\n" + (color?NC:"");

        for (size_t i=0; i<f.size(); i++) {
            if (f[i] == '\n') {
                width = std::max(width, i-pos);
                pos = i;
            }
        }

        std::string s = std::string(width, '=') + f + std::string(width, '=');
        return s;
    } else {
        return this->fmt(0, color);
    }
}

std::string ExprNode::stringify(bool color) {
    return this->str(color) + (color? NC : "");
}


/* Literal */
Literal::Literal(const std::string& s): ExprNode(ExprType::T_LITERAL), escaped(s) {
    if (s.size() > 1 && s[0] == '\\') {
        chars = s.substr(1);
    } else {
        chars = s;
    }
}

Literal::~Literal() {
}

void Literal::append(Literal* rhs) {
    escaped += rhs->escaped;
    chars += rhs->chars;
    delete rhs;
}

std::string Literal::str(bool color) {
    std::string c = color? NC : "";
    return c + escaped;
}

std::string Literal::fmt(int indent, bool color) {
    std::string c = color? NC : "";
    return prefix(indent) + c + "<Literal " + chars + ">";
}

void Literal::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    if (postFn) postFn(this);
}


Escaped::Escaped(const std::string& s): ExprNode(ExprType::T_ESCAPED), ch(s) { }

std::string Escaped::str(bool color) {
    std::string c = color? iter_color() : "";
    return c + ch;
}

std::string Escaped::fmt(int indent, bool color) {
    std::string c = color? iter_color() : "";
    return prefix(indent) + c + "<Escaped " + ch + ">";
}

void Escaped::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    if (postFn) postFn(this);
}


/* Anchor */

Anchor::Anchor(const std::string s):  ExprNode(ExprType::T_ANCHOR), val(s) {
    assert(s == "^" || s == "$" || s == "\\b" || s == "\\B");
}

std::string Anchor::str(bool color) {
    std::string c = color? iter_color() : "";
    return c + val;
}

std::string Anchor::fmt(int indent, bool color) {
    std::string c = color? iter_color() : "";
    return prefix(indent) + c + "<Anchor " + val +">";
}

void Anchor::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    if (postFn) postFn(this);
}


/* Quantifier */

Quantifier::Quantifier(std::string s): ExprNode(ExprType::T_QUANTIFIER), val(s), prev(nullptr) {
    std::pair<int,int> ret;
    lazy = false;
    if (s.size() > 1 && s.back() == '?') lazy = true;
    if (s.front() == '{') {
        if (s.back() == '?') s.pop_back();
        ret = parse_quantifier(s);
    } else {
        ret = parse_quantifier(s[0]);
    }
    min = ret.first;
    max = ret.second;

    if (max < min) {
        yyerror_throw("Numbers out of order in quantifier " + this->str(false));
    }
}

Quantifier::~Quantifier() {
    delete prev;
}

void Quantifier::attach(ExprNode* node) {
    assert(prev == nullptr);
    prev = node;
}

void Quantifier::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    if (prev) prev->travel(preFn, postFn, postorder);
    if (postFn) postFn(this);
}

std::string Quantifier::_str() {
    std::stringstream ss;
    ss << "{" << min;
    if (max >= 0) {
        ss << ",";
        if (max < INF) {
            ss << max;
        }
    }
    ss << "}";
    if (lazy) ss << "?";
    return ss.str();
}

std::string Quantifier::str(bool color) {
    std::string c = color? iter_color() : "";
    std::string s;
    if (prev) s = prev->str(color);
    s += c + val;
    return s;
}

std::string Quantifier::fmt(int indent, bool color) {
    std::string c = color? iter_color() : "";
    std::string s;
    if (prev) s = prev->fmt(indent, color);
    else s = prefix(indent);
    s += c + "<Quantifier " + _str() + ">";
    return s;
}


/* Range */

Range::Range(const std::string& s): ExprNode(ExprType::T_RANGE) {
    auto [a, b] = parse_range(s);
    start = a;
    end = b;
    if (!isValid()) {
        yyerror_throw("[" + this->str(false) + "]" + " Range out of order");
    }
}

Range::Range(const std::string start, const std::string end)
: ExprNode(ExprType::T_RANGE), start(start), end(end) { }

Range::~Range() {}

bool Range::isValid() {
    if (start.size() < end.size()) {
        return true;
    }
    if (start.size() > end.size()) {
        return false;
    }
    if (start.size() == 1) {
        return start[0] <= end[0];
    }
    for (size_t i=0; i<start.size(); i++) {
        char a = std::tolower(start[i]);
        char b = std::tolower(end[i]);
        if (a < b) return true;
        if (a > b) return false;
    }
    return true;
}

void Range::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    if (postFn) postFn(this);
}

std::string Range::str(bool color) {
    std::string s;
    if (color) s += iter_color();
    s += start + "-" + end;
    return s;
}

std::string Range::fmt(int indent, bool color) {
    std::string c = color? iter_color() : "";
    return prefix(indent) + c + "<Range " + start + "-" + end + ">";
}

/* Any */
Any::Any(): ExprNode(ExprType::T_ANY) {}
Any::~Any() {}

void Any::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    if (postFn) postFn(this);
}

std::string Any::str(bool color) {
    return color? iter_color() + "." : ".";
}

std::string Any::fmt(int indent, bool color) {
    std::string c = color? iter_color() : "";
    return prefix(indent) + c + "<Any .>";
}

/* Sequence */
Sequence::Sequence(): ExprNode(ExprType::T_SEQUENCE) {}

Sequence::~Sequence() {
    for (auto p : nodes) {
        delete p;
    }
}

void Sequence::push(ExprNode* node) {
    assert(node);
    if (!nodes.empty() && node->isLiteral() && nodes.back()->isLiteral()) {
        auto p = static_cast<Literal*>(nodes.back());
        auto q = static_cast<Literal*>(node);
        p->append(q);
    } else {
        nodes.push_back(node);
    }
}

void Sequence::append(ExprNode* node) {
    assert(node);
    if (node->isSequence()) {
        auto seq = static_cast<Sequence*>(node);
        for (auto p : seq->nodes) {
            push(p);
        }
        seq->nodes.clear();
        delete seq;
    } else {
        push(node);
    }
}

void Sequence::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    if (postorder) {
        for (auto it=nodes.rbegin(); it!=nodes.rend(); ++it) {
            (*it)->travel(preFn, postFn, postorder);
        }
    } else {
        for (auto node : nodes) {
            node->travel(preFn, postFn, postorder);
        }
    }
    if (postFn) postFn(this);
}

std::string Sequence::str(bool color) {
    std::string t;
    for (auto node : nodes) {
        t += node->str(color);
    }
    return t;
}

std::string Sequence::fmt(int indent, bool color) {
    std::string c = color? NC : "";
    std::string t;
    for (auto node : nodes) {
        t += node->fmt(indent, color) + c;
    }
    return t;
}

/* Class */

Class::Class(ExprNode* seq, bool negative)
: ExprNode(ExprType::T_CLASS), seq(seq), negative(negative) {}

Class::~Class() {
    delete seq;
}

void Class::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    seq->travel(preFn, postFn, postorder);
    if (postFn) postFn(this);
}

std::string Class::str(bool color) {
    std::string c = color? iter_color() : "";
    std::string t = c + "[";
    if (negative) t += "^";
    t += seq->str(color);
    t += c + "]";
    return t;
}

std::string Class::fmt(int indent, bool color) {
    std::string c = color? iter_color() : "";
    std::string t = prefix(indent) + c + (negative? "[Neg-Class" : "[Class");
    t += seq->fmt(indent + this->indent, color);
    t += prefix(indent) + c + "]";
    return t;
}


/* Group */

Group::Group(ExprNode* expr, bool capture)
: ExprNode(ExprType::T_GROUP), expr(expr), capture(capture), id(0) {}

Group::~Group() {
    delete expr;
}

void Group::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    expr->travel(preFn, postFn, postorder);
    if (postFn) postFn(this);
}

std::string Group::str(bool color) {
    std::string c = color? iter_color() : "";
    return c + (capture? "(" : "(?:") + expr->str(color) + c + ")";
}

std::string Group::fmt(int indent, bool color) {
    std::string c = color? iter_color() : "";
    std::string gid = capture? "#" + std::to_string(id) : "";
    std::string s = prefix(indent) + c + "(Group" + gid;  
    s += expr->fmt(indent + this->indent, color);
    s += prefix(indent) + c + gid + ")";
    return s;
}

/* Lookahead */

Lookahead::Lookahead(ExprNode* expr, bool negative)
: ExprNode(ExprType::T_LOOKAHEAD), expr(expr), negative(negative) { }

Lookahead::~Lookahead() {
    delete expr;
}

void Lookahead::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    expr->travel(preFn, postFn, postorder);
    if (postFn) postFn(this);
}

std::string Lookahead::str(bool color) {
    std::string c = color? iter_color() : "";
    std::string s;
    s = c + (negative? "(?!" : "(?=");
    s += expr->str(color);
    s += c + ")";
    return s;
}

std::string Lookahead::fmt(int indent, bool color) {
    std::string c = color? iter_color() : "";
    std::string s = prefix(indent) + c + (negative? "<Neg-Lookahead" : "<Lookahead");
    s += expr->fmt(indent + this->indent, color);
    s += prefix(indent) + c + ">";
    return s;
}


/* Lookbehind */

Lookbehind::Lookbehind(ExprNode* expr, bool negative)
: ExprNode(ExprType::T_LOOKBEHIND), expr(expr), negative(negative) { }

Lookbehind::~Lookbehind() {
    delete expr;
}

void Lookbehind::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    expr->travel(preFn, postFn, postorder);
    if (postFn) postFn(this);
}

std::string Lookbehind::str(bool color) {
    std::string c = color? iter_color() : "";
    std::string s;
    s = c + (negative? "(?<!" : "(?<=");
    s += expr->str(color) + c + ")";
    return s;
}

std::string Lookbehind::fmt(int indent, bool color) {
    std::string c = color? iter_color() : "";
    std::string s = prefix(indent) + c + (negative? "<Neg-Lookbehind" : "<Lookbehind");
    s += expr->fmt(indent + this->indent, color);
    s += prefix(indent) + c + ">";
    return s;
}

/* Or */

Or::Or(ExprNode* left, ExprNode* right): ExprNode(ExprType::T_OR) {
    items.push_back(left);
    items.push_back(right);
}

Or::~Or() {
    for (auto item : items) {
        if (item) delete item;
    }
}

void Or::appendLeft(ExprNode* node) {
    assert(items[0] == nullptr);
    items[0] = node;
}

void Or::append(ExprNode* node) {
    assert(items.size()>=2 && node);

    if (node->isOR()) {
        auto p = static_cast<Or*>(node);
        assert(p->items.front() == nullptr);
        for (size_t i=1; i<p->items.size(); i++) {
            items.push_back(p->items[i]);
        }
        p->items.clear();
        delete node;
        return;
    }

    auto p = items.back();
    if (p) {
        if (p->isSequence()) {
            auto seq = static_cast<Sequence*>(p);
            seq->append(node);
        } else {
            auto seq = new Sequence();
            seq->append(p);
            seq->append(node);
            items.back() = seq;
        }
    } else {
        items.back() = node;
    }
}

std::string Or::str(bool color) {
    std::string c = color? iter_color() : "";
    std::string s;
    for (size_t i=0; i<items.size(); i++) {
        if (i > 0) {
            s += c + '|';
        }
        if (items[i]) {
            s += items[i]->str(color);
        }
    }
    return s;
}

std::string Or::fmt(int indent, bool color) {
    std::string c = color? iter_color() : "";
    std::string s;
    s += prefix(indent) + c + "(";
    for (size_t i=0; i<items.size(); i++) {
        if (i > 0) {
            s += prefix(indent) + c + ") OR (";
        }
        if (items[i]) {
            s += items[i]->fmt(indent + this->indent, color);
        } else {
            s += prefix(indent + this->indent) + (color?NC:"") + "<Empty>";
        }
    }
    s += prefix(indent) + c + ")";
    return s;
}

void Or::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    if (postorder) {
        for (auto it=items.rbegin(); it!=items.rend(); ++it) {
            if (*it) (*it)->travel(preFn, postFn, postorder);
            else {
                if (preFn) preFn(nullptr);
                if (postFn) postFn(nullptr);
            }
        }
    } else {
        for (auto item : items) {
            if (item) item->travel(preFn, postFn, postorder);
            else {
                if (preFn) preFn(nullptr);
                if (postFn) postFn(nullptr);
            }
        }
    }
    if (postFn) postFn(this);
}