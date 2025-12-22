%{
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <FlexLexer.h>
#include "Parser.h"
#include "y.tab.hh"

// #define YYDEBUG 1
#define YYTOKEN_TABLE 1
#define YYERROR_VERBOSE 1
#define YYMALLOC yy_malloc
#define YYFREE yy_free

extern size_t yycolumn;
extern const char* g_yytext;

extern int yylex();
extern void reset_flex(const std::string& text);
extern void lex_parse(const std::string& s);

static std::string g_text;
static ExprRoot* g_expr = nullptr;

static std::unordered_set<void*> yy_allocs;

static void* yy_malloc(size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    yy_allocs.insert(ptr);
    LOG_DEBUG("malloc %p", ptr);
    return ptr;
}

static void yy_free(void* ptr) {
    yy_allocs.erase(ptr);
    LOG_DEBUG("free %p", ptr);
    std::free(ptr);
}

static void yy_destroy() {
    for (void* ptr : yy_allocs) {
        LOG_DEBUG("free %p", ptr);
        std::free(ptr);
    }
    yy_allocs.clear();
}

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
    try {
        g_expr = nullptr;
        g_text = escape(expr);
        reset_flex(g_text);
        int ret = yyparse();
        if (ret) {
            throw std::runtime_error("yyparse failed");
        }
        return std::unique_ptr<ExprRoot>(g_expr);
    } catch (const std::exception& e) {
        if (debug) lex_parse(g_text);
        LOG_DEBUG("Exception occurred, destroy all");
        ExprRoot::destroy();
        yy_destroy();
        g_expr = nullptr;
        // todo: memory leaks
        
        throw;
    }
}
