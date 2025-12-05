#include <unordered_map>
#include <unordered_set>
#include <set>
#include <stack>
#include "Parser.h"

using Token = size_t;
using State = size_t;
using Bits = std::vector<bool>;

#define TOK_EPSILON 0
#define EPSILON "(ε)"

class DFA;

class NFA {

public:
    friend class DFA;

    NFA();

    void generate(ExprNode* expr, bool utf8_encoding);
    void dump(std::ostream& os=std::cout);

private:
    State new_state();
    void add_jump(State a, Token t, State b);
    Token get_token(const std::string& tok);

private:
    std::vector<std::string> tokens;
    std::unordered_map<std::string,Token> tokenId;
    std::vector<std::unordered_map<Token,std::set<State>>> nfa;
    static State state_initial;
    static State state_final;
    static Token tok_epsilon;
};

class DFA {
public:
    DFA(NFA* nfa): nfa(nfa) {}

    void generate();
    void dump(std::ostream& os=std::cout);

private:
    void epsilon_closure(State x, State s);
    Bits find_closure(const Bits& b, Token t);
    void nfa_to_dfa();
    void simplify();
    void add_jump(State a, Token t, State b);
    bool is_valid_state(State s);

private:
    std::vector<std::unordered_map<Token,State>> dfa;
    std::vector<std::unordered_set<State>> epsClosure;
    std::unordered_set<State> terminals;
    std::unordered_set<State> valids;
    std::vector<Bits> nfa_closure; // [dfa state: {nfa states closure...}, ...]
    NFA* nfa;
};
