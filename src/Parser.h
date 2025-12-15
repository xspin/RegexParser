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
#include "utils.h"

#define INF INT_MAX

#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define PURPLE "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define UNDERLINE "\033[04m"
#define NC "\033[0m"

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
    default:
        return "Unknown";
    }
}

extern void yyerror_throw(const std::string& msg);

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
    static int indent;

    ExprNode(ExprType t);

    std::string typeName() {
        return exprTypeName(this->type);
    }

    bool isType(ExprType t);
    bool isQuantifier();
    bool isOR();
    bool isSequence();
    bool isGroup();
    bool isLiteral();

    std::string prefix(int indent);

    virtual std::string format(int indent=2, bool color=false) final;
    virtual std::string stringify(bool color=false) final;
    virtual ~ExprNode() {}
    virtual std::string str(bool color=false) = 0;
    virtual std::string fmt(int indent, bool color) = 0;
    virtual void travel(TravelFunc preFn=nullptr, TravelFunc postFn=nullptr, bool postorder=false) =0;
};


struct Literal: ExprNode {
    std::string escaped;
    std::string chars;

    Literal(const std::string& s);
    ~Literal();
    void append(Literal* rhs);
    std::string str(bool color);
    std::string fmt(int indent, bool color);
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

        \xHH	匹配 ASCII 码为 HH 的字符（HH 为两位十六进制数）
        \uHHHH	匹配 Unicode 码为 HHHH 的字符（HHHH 为四位十六进制数）

        TODO:

        \cX	匹配控制字符
    */
    std::string ch;

    Escaped(const std::string& s);
    std::string str(bool color);
    std::string fmt(int indent, bool color);
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
    std::string fmt(int indent, bool color);
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
};

struct Quantifier: ExprNode {
    std::string val;
    int min;
    int max;
    bool lazy;
    ExprNode* prev;

    Quantifier(std::string s);
    ~Quantifier();
    void attach(ExprNode* node);
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(int indent, bool color);

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
    std::string fmt(int indent, bool color);
};

struct Any: ExprNode {
    Any();
    ~Any();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(int indent, bool color);
};

struct Sequence: ExprNode {
    std::vector<ExprNode*> nodes;

    Sequence();
    ~Sequence();
    void push(ExprNode* node);
    void append(ExprNode* node);
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(int indent, bool color);
};

struct Class: ExprNode {
    ExprNode* seq;
    bool negative;

    Class(ExprNode* seq, bool negative=false);
    ~Class();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(int indent, bool color);
};

struct Group: ExprNode {
    ExprNode* expr;
    bool capture;
    int id;

    Group(ExprNode* expr, bool capture=true);
    ~Group();
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
    std::string str(bool color);
    std::string fmt(int indent, bool color);
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
    std::string fmt(int indent, bool color);
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
    std::string fmt(int indent, bool color);
};

struct Or: ExprNode {
    std::vector<ExprNode*> items;

    Or(ExprNode* left=nullptr, ExprNode* right=nullptr);
    ~Or();
    void appendLeft(ExprNode* node);
    void append(ExprNode* node);
    std::string str(bool color);
    std::string fmt(int indent, bool color);
    void travel(TravelFunc preFn, TravelFunc postFn, bool postorder);
};


static inline void process_groupid(ExprNode* root) {
    static int gid = 1;

    root->travel([](ExprNode* node){
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

extern std::unique_ptr<ExprNode> regex_parse(const std::string& expr, bool debug=false);

#endif