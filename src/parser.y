%{
#include <iostream>
#include <iomanip>
#include <FlexLexer.h>
#include "Parser.h"
#include "y.tab.hh"

// #define YYDEBUG 1
#define YYTOKEN_TABLE 1
#define YYERROR_VERBOSE 1

extern size_t yycolumn;
extern const char* g_yytext;

extern int yylex();
extern void reset_flex(const std::string& text);
extern void lex_parse(const std::string& s);

static std::string g_text;
static ExprRoot* g_expr = nullptr;

void yyerror(const std::string& msg)
{
    int col = (g_yytext && *g_yytext)? yylloc.first_column : yylloc.last_column + 1;
    std::string tok = g_yytext? g_yytext : "";

    std::stringstream ss;
    ss << "Error: " << msg << ", at column " << col;
    if (!tok.empty()) ss << ": Token `" << tok << "` ";

    std::cerr << "\033[31m" << ss.str() << "\033[0m" << std::endl;

    std::string code;
    int err_col;
    if (g_text.size() > 80) {
        size_t a = col;
        std::string prefix;
        if (col > 20) {
            a = col - 20;
            prefix = "... ";
        }
        code = prefix + g_text.substr(a, 40);
        err_col = col - a + prefix.size();
    } else {
        code = g_text;
        err_col = col;
    }

    ss << "\n\n" << code << "\n" << std::setfill('_') << std::setw(err_col) << std::right << "^";

    std::cerr << code << std::endl
        << std::setfill('_') << std::setw(err_col) << std::right << "^" << std::endl;

    throw std::runtime_error(ss.str());
} 

void yyerror_throw(const std::string& msg) {
    yyerror(msg);
}

#ifdef RULE_DEBUG
#define rule_debug(node, msg) do { \
    std::cerr << msg << std::endl  \
        << (static_cast<ExprNode*>(node)->str()) << std::endl << std::endl; \
} while (0)

#else
#define rule_debug(node, msg) do {} while(0)
#endif


%}

%debug

%locations

// yylval
%union {
    ExprNode* expr;
}

%token <expr> LITERAL ANY QUANTIFIER RANGE OR ANCHOR ESCAPED
    LOOKAHEAD LOOKBEHIND NEGLOOKAHEAD NEGLOOKBEHIND NAMEDLPAREN BACKREF

%token LPAREN NLPAREN RPAREN LBRACKET NLBRACKET RBRACKET

%type <expr> expr group term class class_seq optional_q

%%
root: expr {
        g_expr = new ExprRoot($1);
        g_expr->process_groupid();
    };

expr: expr OR {
        auto p = new Or();
        $$ = combine($1, p);
    }
    | expr class {
        $$ = combine($1, $2);
    }
    | expr group {
        $$ = combine($1, $2);
    }
    | expr term {
        $$ = combine($1, $2);
    }
    | class { $$ = $1; }
    | group { $$ = $1; }
    | term { $$ = $1; }
    | OR { $$ = new Or(); }
    ;

optional_q: QUANTIFIER {
        $$ = $1; 
    }
    | { $$ = nullptr; }
    ;

group: LPAREN expr RPAREN optional_q {
        $$ = attach(new Group($2, true), $4);
    }
    | NLPAREN expr RPAREN optional_q {
        $$ = attach(new Group($2, false), $4);
    }
    | NAMEDLPAREN expr RPAREN optional_q {
        auto group = static_cast<Group*>($1);
        group->expr = $2;
        $$ = attach(group, $4);
    }
    | LOOKAHEAD expr RPAREN {
        $$ = new Lookahead($2);
    }
    | NEGLOOKAHEAD expr RPAREN {
        $$ = new Lookahead($2, true);
    }
    | LOOKBEHIND expr RPAREN {
        $$ = new Lookbehind($2);
    }
    | NEGLOOKBEHIND expr RPAREN {
        $$ = new Lookbehind($2, true);
    }
    ;

class: LBRACKET class_seq RBRACKET optional_q {
        $$ = attach(new Class($2, false), $4);
    }
    | NLBRACKET class_seq RBRACKET optional_q {
        $$ = attach(new Class($2, true), $4);
    }
    ; 

class_seq: class_seq LITERAL { $$ = combine($1, $2); }
    | class_seq RANGE { $$ = combine($1, $2); }
    | class_seq ESCAPED { $$ = combine($1, $2); }
    | LITERAL { $$ = $1; }
    | RANGE { $$ = $1; }
    | ESCAPED { $$ = $1; }
    ;

term: LITERAL optional_q { $$ = attach($1, $2); }
    | ANY optional_q { $$ = attach($1, $2); }
    | ESCAPED optional_q { $$ = attach($1, $2); }
    | BACKREF optional_q { $$ = attach($1, $2); }
    | ANCHOR { $$ = $1; }
    ;
%%

std::unique_ptr<ExprRoot> regex_parse(const std::string& expr, bool debug) {
    if (expr.empty()) {
        throw std::runtime_error("Empty Expr!");
    }
    g_expr = nullptr;
    g_text = escape(expr);
    reset_flex(g_text);
    int ret = yyparse();
    if (ret) {
        if (debug) lex_parse(g_text);
        if (g_expr) delete g_expr; 
        return nullptr;
    }
    return std::unique_ptr<ExprRoot>(g_expr);
}
