#ifndef __DFA_H__
#define __DFA_H__

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <stack>
#include "Parser.h"
#include "GraphBox.h"

using Token = size_t;
using State = size_t;
using Bits = std::vector<bool>;

#define TOK_EPSILON 0
#define EPSILON "ε"
#define TOK_INVALID INT_MAX

class DFA;

class NFA {

public:
    friend class DFA;

    NFA(bool color);

    void generate(ExprNode* expr, bool utf8_encoding);
    void dump(std::ostream& os=std::cout);

private:
    State new_state();
    void add_jump(State a, Token t, State b);
    Token get_token(const std::string& tok);
    void simplify();

private:
    std::vector<std::string> tokens;
    std::unordered_map<std::string,Token> tokenId;
    std::vector<std::unordered_map<Token,std::set<State>>> nfa; // state: {tok:[state...]} ...
    static State state_initial;
    static State state_final;
    static Token tok_epsilon;
    bool color;
};

class DFAGraph;

class DFA {
public:
    friend class DFAGraph;

    DFA(NFA* nfa): nfa(nfa), state_initial(0) {
    }

    void generate();
    void dump(std::ostream& os=std::cout);
    bool is_valid(State s);
    bool is_terminal(State s);
    State next(State s, Token t);
    const std::unordered_map<Token,State>* next(State s);
    size_t states();
    std::string token_name(Token t);
    const std::vector<std::string> get_tokens();

private:
    bool is_color();
    void epsilon_closure(State x, State s);
    Bits find_closure(const Bits& b, Token t);
    void nfa_to_dfa();
    void simplify();
    void add_jump(State a, Token t, State b);

private:
    std::vector<std::unordered_map<Token,State>> dfa;
    std::vector<std::unordered_set<State>> epsClosure;
    std::unordered_set<State> terminals;
    std::unordered_set<State> valids;
    std::vector<Bits> nfa_closure; // [dfa state: {nfa states closure...}, ...]
    NFA* nfa;
    State state_initial;
};

class DFAGraph {
public:
    DFAGraph(DFA* dfa);
    void render();
    void dump(std::ostream& os);

private:
    std::vector<std::pair<int,int>> init_canvas(const std::vector<std::vector<State>> &paths,
        std::pair<size_t,size_t> wxh, std::pair<size_t,size_t> line_size);

    void draw_links(Block& convas,std::vector<std::pair<int,int>> &positions,
        std::pair<size_t,size_t>width_height, std::pair<size_t,size_t> line_size);

    std::vector<std::vector<State>> find_paths();

    void to_rows(size_t block_width, size_t block_height,
        const std::vector<std::pair<int,int>>& pos);

private:
    DFA* dfa;
    Rows rows;
    Block canvas;
};

/*
(a[ab]c|b[bc]c|c[ac]c)
(aab|aac|aba|abc|aca|acb|baa|bac|bba|bbc|bca|bcb|caa|cab|cba|cbc)
a(bc|cb)+a|b(ac|ca)+b|c(ab|ba)+c
(ab|ac|ba|bc|ca|cb){2,4}
a+bc*|b+ca*|c+ab*

*/

#endif // __DFA_H__