#include "Parser.h"

extern void yyerror_throw(const std::string& msg);

int ExprNode::indent_ = 0;
int ExprNode::depth_ = 0;
std::unordered_set<void*> ExprNode::allocs_;

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

    a = i > 0 ? stoi(t.substr(0, i)) : 0;
    b = i == t.size()-1 ? INF : stoi(t.substr(i+1));

    return {a, b};
}

static inline std::pair<std::string,std::string> parse_range(const std::string& s) {
    size_t i = s.substr(1, s.size()-2).find('-') + 1;
    std::string a = s.substr(0, i);
    std::string b = s.substr(i+1);
    return {a, b};
}

/* ExprNode */

ExprNode::ExprNode(ExprType t): type(t) { }

bool ExprNode::isType(ExprType t) {
    return type == t;
}

bool ExprNode::isQuantifier() {
    return isType(ExprType::T_QUANTIFIER);
}

bool ExprNode::isOR() {
    return isType(ExprType::T_OR);
}

bool ExprNode::isSequence() {
    return isType(ExprType::T_SEQUENCE);
}

bool ExprNode::isGroup() {
    return isType(ExprType::T_GROUP);
}

bool ExprNode::isClass() {
    return isType(ExprType::T_CLASS);
}

bool ExprNode::isRoot() {
    return isType(ExprType::T_ROOT);
}


bool ExprNode::isLiteral() {
    return isType(ExprType::T_LITERAL);
}

std::string ExprNode::prefix() {
    if (this->indent_ > 0) {
        return "\n" + std::string(depth_ * indent_, ' ');
    } else {
        return "";
    }
}

void* ExprNode::operator new(std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    allocs_.insert(ptr);
    return ptr;
}

void* ExprNode::operator new[](std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    allocs_.insert(ptr);
    return ptr;
}

void ExprNode::operator delete(void* ptr) noexcept {
    if (allocs_.find(ptr) != allocs_.end()) {
        allocs_.erase(ptr);
        std::free(ptr);
    }
}

void ExprNode::operator delete[](void* ptr) noexcept {
    if (allocs_.find(ptr) != allocs_.end()) {
        allocs_.erase(ptr);
        std::free(ptr);
    }
}

void ExprNode::destroy() noexcept {
    for (auto ptr : allocs_) {
        std::free(ptr);
    }
    allocs_.clear();
}

/* ExprRoot */
ExprRoot::ExprRoot(ExprNode* expr): ExprNode(ExprType::T_ROOT), expr(expr) { }

ExprRoot::~ExprRoot() {
    delete expr;
}

std::string ExprRoot::str(bool color) {
    return expr->str(color);
}

std::string ExprRoot::fmt(bool color) {
    return expr->fmt(color);
}

std::string ExprRoot::xml() {
    depth_ = 0;
    indent_ = 2;
    std::string s = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    ++depth_;
    s += "\n<RegularExpression expr=\"" + escape_xml(stringify(false)) + "\">"
      + expr->xml() +  "\n</RegularExpression>";
    --depth_;
    return s;
}

void ExprRoot::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    expr->travel(preFn, postFn, postorder);
}

std::string ExprRoot::stringify(bool color) {
    reset_color();
    return this->str(color) + (color? NC : "");
}

std::string ExprRoot::format(int indent, bool color) {
    depth_ = 0;
    reset_color();
    this->indent_ = indent;
    if (indent > 0) {
        size_t pos = 0;
        size_t width = 0;
        std::string f = this->fmt(color) + "\n" + (color?NC:"");

        for (size_t i=0; i<f.size(); i++) {
            if (f[i] == '\n') {
                width = std::max(width, i-pos);
                pos = i;
            }
        }
        return f;
    } else {
        return this->fmt(color);
    }
}

void ExprRoot::process_groupid() {
    int gid = 1;

    expr->travel([&gid](ExprNode* node){
        if (node && node->isGroup()) {
            auto p = static_cast<Group*>(node);
            if (p->capture) {
                p->id = gid++;
            } else {
                p->id = 0;
            }
        }
    });
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

std::string Literal::fmt(bool color) {
    std::string c = color? NC : "";
    return prefix() + c + "<Literal " + chars + ">";
}

std::string Literal::xml() {
    return prefix() + "<Literal>" + escape_xml(chars) +  "</Literal>";
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

std::string Escaped::fmt(bool color) {
    std::string c = color? iter_color() : "";
    return prefix() + c + "<Escaped " + ch + ">";
}

std::string Escaped::xml() {
    std::string s = prefix();
    std::string tag;
    if (ch.size() == 4 && ch.substr(0, 2) == "\\x") {
        tag = "Hex";
    } else if (ch.size() == 4 && ch.substr(0, 2) == "\\0") {
        tag = "Octal";
    } else if (ch.size() >= 4 && (ch.substr(0, 2) == "\\u" || ch.substr(0, 2) == "\\U")) {
        tag = "Unicode";
    } else {
        tag = "Escaped";
    }
    return prefix() + "<" + tag + ">" + escape_xml(ch) +  "</" + tag + ">";
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

std::string Anchor::fmt(bool color) {
    std::string c = color? iter_color() : "";
    return prefix() + c + "<Anchor " + val +">";
}

std::string Anchor::xml() {
    std::string s;
    if (val == "^") {
        s = "<StartOfLine/>";
    } else if (val == "$") {
        s = "<EndOfLine/>";
    } else if (val == "\\b") {
        s = "<WordBoundary/>";
    } else if (val == "\\B") {
        s = "<NonWordBoundary/>";
    } else {
        s = "<Anchor>" + val +  "</Anchor>";
    }
    return prefix() + s;
}

void Anchor::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    if (postFn) postFn(this);
}


/* Quantifier */

Quantifier::Quantifier(std::string s): ExprNode(ExprType::T_QUANTIFIER), val(s), prev(nullptr) {
    std::pair<int,int> ret;
    tag = QuantifierTag::GREEDY;
    if (s.size() > 1) {
        if (s.back() == '+') tag = QuantifierTag::POSSESSIVE;
        else if (s.back() == '?') tag = QuantifierTag::LAZY;
    }
    if (s.front() == '{') {
        if (tag != QuantifierTag::GREEDY) s.pop_back();
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
    if (prev) delete prev;
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
    if (tag == QuantifierTag::LAZY) ss << "?";
    else if (tag == QuantifierTag::POSSESSIVE) ss << "+";
    return ss.str();
}

std::string Quantifier::str(bool color) {
    std::string c = color? iter_color() : "";
    std::string s;
    if (prev) s = prev->str(color);
    s += c + val;
    return s;
}

std::string Quantifier::fmt(bool color) {
    std::string c = color? iter_color() : "";
    std::string s;
    if (prev) s = prev->fmt(color);
    else s = prefix();
    s += c + "<Quantifier " + _str() + ">";
    return s;
}

std::string Quantifier::xml() {
    std::stringstream ss;
    ss << prefix() << "<Quantifier" << " min=\"" << min << '"'  << " max=\"";
    if (max < INF) {
        ss << max;
    } else {
        ss << "Infinity";
    }
    ss << "\" mode=\"";
    if (tag == QuantifierTag::LAZY) ss << "lazy";
    else if (tag == QuantifierTag::POSSESSIVE) ss << "possessive";
    else ss << "greedy";
    ss  << '"'<< ">";

    ++depth_;
    ss << prev->xml();
    --depth_;

    ss << prefix() <<  "</Quantifier>";
    return ss.str();
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

std::string Range::fmt(bool color) {
    std::string c = color? iter_color() : "";
    return prefix() + c + "<Range " + start + "-" + end + ">";
}

std::string Range::xml() {
    std::string s = prefix() + "<Range";
    s += " start=\"" + escape_xml(start) + "\"";
    s += " end=\"" + escape_xml(end) + "\"/>";
    return s;
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

std::string Any::fmt(bool color) {
    std::string c = color? iter_color() : "";
    return prefix() + c + "<Any .>";
}

std::string Any::xml() {
    return prefix() + "<Any/>";
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

std::string Sequence::fmt(bool color) {
    std::string c = color? NC : "";
    std::string t;
    for (auto node : nodes) {
        t += node->fmt(color) + c;
    }
    return t;
}

std::string Sequence::xml() {
    std::string s;
    for (auto node : nodes) {
        s += node->xml();
    }
    return s;
}

/* Class */

Class::Class(ExprNode* seq, bool negative)
: ExprNode(ExprType::T_CLASS), seq(seq), negative(negative) {}

Class::~Class() {
    if (seq) delete seq;
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

std::string Class::fmt(bool color) {
    std::string c = color? iter_color() : "";
    std::string t = prefix() + c + (negative? "[Neg-Class" : "[Class");
    depth_++;
    t += seq->fmt(color);
    depth_--;
    t += prefix() + c + "]";
    return t;
}

std::string Class::xml() {
    std::string s = prefix() + "<Class";
    if (negative) s += " negative=\"true\"";
    s += ">";
    ++depth_;
    s += seq->xml();
    --depth_;
    s += prefix() + "</Class>";
    return s;
}

/* Group */

Group::Group(ExprNode* expr, bool capture, const std::string& name)
: ExprNode(ExprType::T_GROUP), expr(expr), capture(capture), id(0), name(name) {}

Group::~Group() {
    if (expr) delete expr;
}

void Group::travel(TravelFunc preFn, TravelFunc postFn, bool postorder) {
    if (preFn) preFn(this);
    expr->travel(preFn, postFn, postorder);
    if (postFn) postFn(this);
}

std::string Group::str(bool color) {
    std::string c = color? iter_color() : "";
    if (name.empty()) {
        return c + (capture? "(" : "(?:") + expr->str(color) + c + ")";
    } else {
        return c + "(?<" + name + ">" + expr->str(color) + c + ")";
    }
}

std::string Group::fmt(bool color) {
    std::string c = color? iter_color() : "";
    std::string gid = capture? "#" + std::to_string(id) : "";
    if (!name.empty()) gid += "<" + name + ">";
    std::string s = prefix() + c + "(Group" + gid;  
    depth_++;
    s += expr->fmt(color);
    depth_--;
    s += prefix() + c + gid + ")";
    return s;
}

std::string Group::xml() {
    std::string s = prefix() + "<Group";
    if (capture) {
        s += " id=\"" + std::to_string(id) + "\"";
        if (!name.empty()) {
            s += " name=\"" + name + "\"";
        }
    } else {
        s += " capture=\"false\"";
    }
    s += ">";
    ++depth_;
    s += expr->xml();
    --depth_;
    s += prefix() + "</Group>";
    return s;
}


/* Backref */
Backref::Backref(int id, const std::string& name)
: ExprNode(ExprType::T_BACKREF), id(id), name(name) { }

Backref::~Backref() { }

void Backref::travel(TravelFunc preFn, TravelFunc postFn, bool postorder)
{
    if (preFn) preFn(this);
    if (postFn) postFn(this);
}

std::string Backref::str(bool color)
{
    std::string c = color? iter_color() : "";
    if (name.empty()) {
        return c + "\\" + std::to_string(id);
    } else {
        return c + "\\k<" + name + ">";
    }
}

std::string Backref::fmt(bool color)
{
    std::string c = color? iter_color() : "";
    std::string s = prefix() + c + "<Ref " + str(false) + ">";
    return s;
}

std::string Backref::xml() {
    std::string s = prefix() + "<Reference";
    if (name.empty()) {
        s += " id=\"" + std::to_string(id) + "\"";
    } else {
        s += " name=\"" + name + "\"";
    }
    s += "/>";
    return s;
}

/* Lookahead */

Lookahead::Lookahead(ExprNode* expr, bool negative)
: ExprNode(ExprType::T_LOOKAHEAD), expr(expr), negative(negative) { }

Lookahead::~Lookahead() {
    if (expr) delete expr;
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

std::string Lookahead::fmt(bool color) {
    std::string c = color? iter_color() : "";
    std::string s = prefix() + c + (negative? "<Neg-Lookahead" : "<Lookahead");
    ++depth_;
    s += expr->fmt(color);
    --depth_;
    s += prefix() + c + ">";
    return s;
}

std::string Lookahead::xml() {
    std::string s = prefix() + "<Lookahead";
    if (negative) s += " negative=\"true\"";
    s += ">";
    ++depth_;
    s += expr->xml();
    --depth_;
    s += prefix() + "</Lookahead>";
    return s;
}

/* Lookbehind */

Lookbehind::Lookbehind(ExprNode* expr, bool negative)
: ExprNode(ExprType::T_LOOKBEHIND), expr(expr), negative(negative) { }

Lookbehind::~Lookbehind() {
    if (expr) delete expr;
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

std::string Lookbehind::fmt(bool color) {
    std::string c = color? iter_color() : "";
    std::string s = prefix() + c + (negative? "<Neg-Lookbehind" : "<Lookbehind");
    ++depth_;
    s += expr->fmt(color);
    --depth_;
    s += prefix() + c + ">";
    return s;
}

std::string Lookbehind::xml() {
    std::string s = prefix() + "<Lookbehind";
    if (negative) s += " negative=\"true\"";
    s += ">";
    ++depth_;
    s += expr->xml();
    --depth_;
    s += prefix() + "</Lookbehind>";
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

std::string Or::fmt(bool color) {
    std::string c = color? iter_color() : "";
    std::string s;
    s += prefix() + c + "(";
    for (size_t i=0; i<items.size(); i++) {
        if (i > 0) {
            s += prefix() + c + ") OR (";
        }
        ++depth_;
        if (items[i]) {
            s += items[i]->fmt(color);
        } else {
            s += prefix() + (color?NC:"") + "<Empty>";
        }
        --depth_;
    }
    s += prefix() + c + ")";
    return s;
}

std::string Or::xml() {
    std::string s = prefix() + "<Alternation>";
    ++depth_;
    for (auto item : items) {
        if (item) {
            s += item->xml();
        } else {
            s += prefix() + "<Empty/>";
        }
    }
    --depth_;
    s += prefix() + "</Alternation>";
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
