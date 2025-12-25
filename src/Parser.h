#ifndef __PARSER_H__
#define __PARSER_H__

#include <cassert>
#include <memory>
#include <cstdint>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <vector>
#include <functional>
#include <climits>  
#include <unordered_set>
#include "utils.h"

#define INF INT_MAX

#define ESC_PRE '\033'
#define BLACK "\033[90m"
#define RED "\033[91m"
#define GREEN "\033[92m"
#define YELLOW "\033[93m"
#define BLUE "\033[94m"
#define PURPLE "\033[95m"
#define CYAN "\033[96m"
#define WHITE "\033[97m"
#define UNDERLINE "\033[04m"
#define NC "\033[00m"


static inline std::string esc_code_color(const std::string& code) {
    std::string color;
    if (code == RED) {
        color = "#EF4444";
    } else if (code == GREEN) {
        color = "#22C55E";
    } else if (code == YELLOW) {
        color = "#F59E0B";
    } else if (code == BLUE) {
        color = "#3B82F6";
    } else if (code == PURPLE) {
        color = "#8B5CF6";
    } else if (code == CYAN) {
        color = "#06B6D4";
    } else if (code == BLACK) {
        color = "black";
    } else if (code == WHITE) {
        color = "white";
    }
    return color;
}

static inline std::string escape_xml(const std::string& s) {
    std::string res;
    for (char ch : s) {
        switch (ch) {
            case '&': res += "&amp;"; break;
            case '<': res += "&lt;"; break;
            case '>': res += "&gt;"; break;
            case '"': res += "&quot;"; break;
            case '\'': res += "&apos;"; break;
            default: res += ch; break;
        }
    }
    return res;
}

enum class ExprType {
    T_LITERAL,
    T_QUANTIFIER,
    T_RANGE,
    T_ANY,
    T_SEQUENCE,
    T_CLASS,
    T_GROUP,
    T_OR,
    T_ANCHOR,
    T_LOOKAHEAD,
    T_LOOKBEHIND,
    T_ESCAPED,
    T_BACKREF,
    T_ROOT,
};

static inline std::string exprTypeName(ExprType t) {
    switch (t)
    {
    case ExprType::T_LITERAL: return "Literal";
    case ExprType::T_QUANTIFIER: return "Quantifier";
    case ExprType::T_RANGE: return "Range";
    case ExprType::T_ANY: return "Any";
    case ExprType::T_SEQUENCE: return "Sequence";
    case ExprType::T_CLASS: return "Class";
    case ExprType::T_GROUP: return "Group";
    case ExprType::T_OR: return "Or";
    case ExprType::T_ANCHOR: return "Anchor";
    case ExprType::T_LOOKAHEAD: return "Lookahead";
    case ExprType::T_LOOKBEHIND: return "Lookbehind";
    case ExprType::T_ESCAPED: return "Escaped";
    case ExprType::T_BACKREF: return "Backreference";
    case ExprType::T_ROOT: return "Root";
    default:
        return "Unknown";
    }
}


void reset_color();
std::string iter_color();

static inline std::string end_color() {
    return NC;
}

static inline std::string iter_color_pack(const std::string s) {
    return iter_color() + s + NC;
}



struct ExprNode {
    using TravelFunc = std::function<void(ExprNode*)>;

    ExprType type;

    ExprNode(ExprType t);

    std::string typeName() {
        return exprTypeName(this->type);
    }

    bool isType(ExprType t);
    bool isQuantifier();
    bool isOR();
    bool isSequence();
    bool isGroup();
    bool isClass();
    bool isRoot();
    bool isLiteral();

    std::string prefix();

    virtual ~ExprNode() {}
    virtual std::string str(bool color=false) = 0;
    virtual std::string fmt(bool color) = 0;
    virtual std::string xml() = 0;
    virtual void travel(TravelFunc preFn=nullptr, TravelFunc postFn=nullptr, bool postorder=false) =0;

    void* operator new(std::size_t size);
    void* operator new[](std::size_t size);
    void operator delete(void* ptr) noexcept;
    void operator delete[](void* ptr) noexcept;
    static void destroy() noexcept;

    static int indent_;
    static int depth_;
    static std::unordered_set<void*> allocs_;
};

struct ExprRoot: ExprNode {
    ExprNode* expr;

    ExprRoot(ExprNode* expr);
    ~ExprRoot();

    std::string stringify(bool color=false);
    std::string format(int indent, bool color);
    std::string xml();

    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    void process_groupid();

private:
    std::string str(bool color);
    std::string fmt(bool color);
};

struct Literal: ExprNode {
    std::string escaped;
    std::string chars;

    Literal(const std::string& s);
    ~Literal();
    void append(Literal* rhs);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
};


struct Escaped: ExprNode {
    /*
        \d  [0-9]
        \D  [^0-9]
        \w  [A-Za-z0-9_]	
        \W  [^A-Za-z0-9_]	
        \s  [ \t\n\r\f\v]
        \S  [^ \t\n\r\f\v]
        \n  换行符（Line Feed）
        \t  回车符（Carriage Return）
        \t  制表符（Tab）
        \f  换页符（Form Feed）
        \v  垂直制表符（Vertical Tab）
        \0  空字符（Null）

        \0xx (0<=x<8) 8进制数

        \xHH	匹配 ASCII 码为 HH 的字符（HH 为两位十六进制数）
        \uHHHH	匹配 Unicode 码为 HHHH 的字符（HHHH 为四位十六进制数）
        \UHHHHHHHH 32位Unicode

        \cX	匹配控制字符
    */
    std::string ch;

    Escaped(const std::string& s);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    bool isUnicode() {
        return ch.size() == 6 && ch.substr(0,2) == "\\u";
    }
    bool isHex() {
        return ch.size() == 4 && ch.substr(0,2) == "\\x";
    }
};

struct Anchor: ExprNode {
    /*
        \b          word boundary
        \B          not a word boundary
        ^           start of subject
        $           end of subject
    */
    std::string val;

    Anchor(const std::string s);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
};

enum class QuantifierTag {
    GREEDY,
    POSSESSIVE, // ...+
    LAZY, // ...?
};

struct Quantifier: ExprNode {
    std::string val;
    int min;
    int max;
    QuantifierTag tag;
    ExprNode* prev;

    Quantifier(std::string s);
    ~Quantifier();
    void attach(ExprNode* node);
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();

private:
    std::string _str();

};

struct Range: ExprNode {
    std::string start;
    std::string end;

    Range(const std::string& s);
    Range(const std::string start, const std::string end);
    ~Range();
    bool isValid();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
};

struct Any: ExprNode {
    Any();
    ~Any();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
};

struct Sequence: ExprNode {
    std::vector<ExprNode*> nodes;

    Sequence();
    ~Sequence();
    void push(ExprNode* node);
    void append(ExprNode* node);
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
};

struct Class: ExprNode {
    ExprNode* seq;
    bool negative;

    Class(ExprNode* seq, bool negative=false);
    ~Class();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
};

struct Group: ExprNode {
    ExprNode* expr;
    bool capture;
    int id;
    std::string name;

    Group(ExprNode* expr, bool capture=true, const std::string& name="");
    ~Group();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
};

struct Backref: ExprNode {
    int id;
    std::string name;

    Backref(int id, const std::string& name="");
    ~Backref();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
};

struct Lookahead: ExprNode {
    /*
        ...(?=...) positive lookahead
        ...(?!...) negative lookahead
     */
    ExprNode* expr;
    bool negative;

    Lookahead(ExprNode* expr, bool negative=false);
    ~Lookahead();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
};


struct Lookbehind: ExprNode {
    /*
        (?<=...)... positive lookbehind
        (?<!...)... negative lookbehind
     */
    ExprNode* expr;
    bool negative;

    Lookbehind(ExprNode* expr, bool negative=false);
    ~Lookbehind();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
};

struct Or: ExprNode {
    std::vector<ExprNode*> items;

    Or(ExprNode* left=nullptr, ExprNode* right=nullptr);
    ~Or();
    void appendLeft(ExprNode* node);
    void append(ExprNode* node);
    std::string str(bool color);
    std::string fmt(bool color);
    std::string xml();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
};




static inline ExprNode* attach(ExprNode* a, ExprNode* b) {
    if (!b) return a;
    assert(b->isQuantifier());
    static_cast<Quantifier*>(b)->attach(a);
    return b;
}

static inline ExprNode* combine(ExprNode* a, ExprNode* b) {
    if (!b) return a;
    if (!a) return b;

    if (a->isOR()) {
        auto p = static_cast<Or*>(a);
        p->append(b);
        return a;
    } else if (b->isOR()) {
        auto p = static_cast<Or*>(b);
        p->appendLeft(a);
        return b;
    }

    Sequence* seq;
    if (a->isSequence()) {
        seq = static_cast<Sequence*>(a);
        seq->append(b);
    } else {
        seq = new Sequence();
        seq->append(a);
        seq->append(b);
    }
    return seq;
}


static inline std::string escape(const std::string& str) {
    std::string s;
    for (char c : str) {
        switch (c) {
            case '\n': s += "\\n"; break;
            case '\r': s += "\\r"; break;
            case '\t': s += "\\t"; break;
            case '\f': s += "\\f"; break;
            case '\v': s += "\\v"; break;
            case '\a': s += "\\a"; break;
            case '\e': s += "\\e"; break;
            case '\0': s += "\\0"; break;
            default: s += c;
        }
    }
    return s;
}

extern std::unique_ptr<ExprRoot> regex_parse(const std::string& expr, bool debug=false);

#endif