%option noyywrap warn c++

%{
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "Parser.h"
#include "y.tab.hh"
#include "utils.h"

#define COMMON() \
    g_yytext = yytext; \
    yylloc.first_column = yylloc.last_column + 1; \
    yylloc.last_column += yyleng;

static std::unique_ptr<yyFlexLexer> g_lexer;
static std::unique_ptr<std::istringstream> g_iss;
const char* g_yytext;

void reset_flex(const std::string& text) {
    g_iss = std::make_unique<std::istringstream>(text);
    std::istream *s = static_cast<std::istream*>(g_iss.get());
    g_lexer = std::make_unique<yyFlexLexer>(s);
    yylloc = {0};
    g_yytext = nullptr;
}

int yylex() {
    assert(g_lexer);
    return g_lexer->yylex();
}

%}

%x CLASS

escaped_literal \\[\\.*+?()\[\]{}|^$]
special_escaped \\x[0-9a-fA-F]{2}|\\u[0-9a-fA-F]{4}|\\c[a-zA-Z]|\\0[0-7]{2}|\\[dDwWsStnrfv0]
range_expr \\0[0-7]{2}-\\0[0-7]{2}|\\x[0-9a-fA-F]{2}-\\x[0-9a-fA-F]{2}|\\u[0-9a-fA-F]{4}-\\u[0-9a-fA-F]{4}|([^\\\[]|\\.)-([^\\\]]|\\.)|(\\c[a-zA-Z])-(\\c[a-zA-Z])

%%

{escaped_literal} {
    COMMON();
    yylval.expr = new Literal(yytext);
    return LITERAL;
}

{special_escaped} {
    COMMON();
    yylval.expr = new Escaped(yytext);
    return ESCAPED;
}

\\[1-9][0-9]{0,5} {
    COMMON();
    std::string s = yytext;
    int id = std::stoi(s.substr(1));
    yylval.expr = new Backref(id);
    return BACKREF;
}

\\k<([a-zA-Z_][0-9a-zA-Z_]*|[0-9]+)> {
    COMMON();
    std::string s = yytext;
    yylval.expr = new Backref(0, s.substr(3, s.size()-4));
    return BACKREF;
}

\(\?[=!] {
    COMMON();
    yylval.expr = nullptr;
    return yytext[2] == '=' ? LOOKAHEAD : NEGLOOKAHEAD;
}


\(\?<[=!] {
    COMMON();
    yylval.expr = nullptr;
    return yytext[3] == '=' ? LOOKBEHIND : NEGLOOKBEHIND;
}

\(\?<[a-zA-Z_][0-9a-zA-Z_]*> {
    // named group
    COMMON();
    std::string s = yytext;
    s = s.substr(3);
    s.pop_back();
    yylval.expr = new Group(nullptr, true, s);
    return NAMEDLPAREN;
}

"(?:" {
    // no capture group start
    COMMON();
    yylval.expr = nullptr;
    return NLPAREN;
}

"(" {
    // group start
    COMMON();
    yylval.expr = nullptr;
    return LPAREN;
}

")" {
    COMMON();
    yylval.expr = nullptr;
    return RPAREN;
}

"[^" {
    BEGIN(CLASS);
    COMMON();
    yylval.expr = nullptr;
    return NLBRACKET;
}

"[" {
    BEGIN(CLASS);
    COMMON();
    yylval.expr = nullptr;
    return LBRACKET;
}

<CLASS>{range_expr} {
    COMMON();
    yylval.expr = new Range(yytext);
    return RANGE;
}

<CLASS>{escaped_literal} {
    COMMON();
    yylval.expr = new Literal(yytext);
    return LITERAL;
}

<CLASS>{special_escaped} {
    COMMON();
    yylval.expr = new Escaped(yytext);
    return ESCAPED;
}

<CLASS>\\. {
    COMMON();
    yylval.expr = new Literal(yytext);
    return LITERAL;
}

<CLASS>"]" {
    BEGIN(INITIAL);
    COMMON();
    yylval.expr = nullptr;
    return RBRACKET;
}
<CLASS>. {
    COMMON();
    yylval.expr = new Literal(yytext);
    return LITERAL;
}

\$|\^|\\b|\\B {
    COMMON();
    yylval.expr = new Anchor(yytext);
    return ANCHOR;
}

\\. {
    COMMON();
    yylval.expr = new Literal(yytext);
    return LITERAL;
}

[?*+][+?]? {
    COMMON();
    yylval.expr = new Quantifier(yytext);
    return QUANTIFIER;
}

\{[0-9]+\}[+?]? {
    COMMON();
    yylval.expr = new Quantifier(yytext);
    return QUANTIFIER;
}

\{([0-9]+)?,([0-9]+)?\}[+?]? {
    COMMON();
    yylval.expr = new Quantifier(yytext);
    return QUANTIFIER;
}

"." {
    COMMON();
    yylval.expr = new Any();
    return ANY;
}

\s+ {
    COMMON();
    yylval.expr = new Literal(yytext);
    return LITERAL;
}

"|" {
    COMMON();
    yylval.expr = nullptr;
    return OR;
}

. {
    COMMON();
    yylval.expr = new Literal(yytext);
    return LITERAL;
}

%% 


static inline const char* tokenstr(int t) {
    switch (t) {
        case LITERAL: return "LITERAL";
        case ANY: return "ANY";
        case QUANTIFIER: return "QUANTIFIER";
        case NAMEDLPAREN: return "NAMEDLPAREN";
        case BACKREF: return "BACKREF";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case NLBRACKET: return "NLBRACKET";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case RANGE: return "RANGE";
        case OR: return "OR";
        case ANCHOR: return "ANCHOR";
        case ESCAPED: return "ESCAPED";
        case LOOKAHEAD: return "LOOKAHEAD";
        case LOOKBEHIND: return "LOOKBEHIND";
        case NEGLOOKAHEAD: return "!LOOKAHEAD";
        case NEGLOOKBEHIND: return "!LOOKBEHIND";
        default: return "UNKNOWN";
    };
}

void lex_parse(const std::string& s) {
    std::cout << "\nLex: /" << s << "/" << std::endl;
    reset_flex(s);
    int tok;
    std::cout << "Col TokId TokName  Text    Lex\n";
    while((tok = yylex()) > 0) {
        std::cout 
            << std::setw(2) << std::right << yylloc.first_column << ": " << tok << "  "
            << std::setw(12) << std::left << tokenstr(tok)
            << std::setw(5) << std::left  << g_yytext
            << " : " 
            << (yylval.expr?yylval.expr->str():"NULL")
            << "\n";
    }
}

#ifdef LEXER_BIN

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <RegularExpression>" << std::endl;
        return 1;
    }
    for (int i=1; i<argc; i++) {
	    lex_parse(argv[i]);
    }
    return 0;
}

#endif