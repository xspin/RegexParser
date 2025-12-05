#include "iostream"
#include "unicode.h"
#include "DFA.h"

static std::string special_token(const std::string& s) {
    return "(" + s + ")";
}

Token NFA::tok_epsilon = 0;
State NFA::state_initial = 0;
State NFA::state_final = 1;

NFA::NFA() {
    tokenId[EPSILON] = TOK_EPSILON;
    tokens.push_back(EPSILON);

    nfa.push_back({}); // start state
    nfa.push_back({}); // end state
}

State NFA::new_state() {
    State s = nfa.size();
    nfa.push_back({});
    return s;
}

void NFA::add_jump(State a, Token t, State b) {
    nfa[a][t].insert(b);
};


Token NFA::get_token(const std::string& tok) {
    auto it = tokenId.find(tok);
    if (it == tokenId.end()) {
        Token t = tokens.size();
        tokenId[tok] = t;
        tokens.push_back(tok);
        return t;
    } else {
        return it->second;
    }
};

void NFA::dump(std::ostream& os) {
    std::string border(20, '#');
    os << "\n" << border << " NFA Start " << border << "\n";
    size_t w = 10;
    for (State s = 0; s < nfa.size(); s++) {
        std::string ss = std::to_string(s);
        if (s == 0) ss += ">";
        else if (s == 1) ss += "*";
        os << std::setw(w) << std::right << ("State " + ss + ":") << "\n";
        for (auto it = nfa[s].begin(); it != nfa[s].end(); ++it) {
            Token t = it->first;
            int d = tokens[t].size() - visual_width(tokens[t]);
            os << std::setw(w+d-2) << std::right << (tokens[t] + ":") << " ";
            bool first = true;
            for (State next : it->second) {
                if (!first) os << ", ";
                os << std::setw(2) << next;
                first = false;
            }
            os << "\n";
        }
        os << "\n";
    }
    os << border << "  NFA End  " << border << "\n";
}

void NFA::generate(ExprNode* expr, bool utf8_encoding) {
    assert(expr);

    State start = 0;
    State end = 1;

    std::stack<std::pair<std::pair<State,State>,int>> stk; // begin state, next state, flag

    auto new_pair = [](State a, State b, int flag=0) -> std::pair<std::pair<State,State>,int> {
        return std::make_pair(std::make_pair(a, b), flag);
    };

    stk.push(new_pair(start, end));

    std::function<void(ExprNode*)> fn = [&](ExprNode* node) {
        if (stk.empty()) {
            DEBUG_OS << "Empty stk! node " << (node?node->typeName() : "NULL") << "\n";
            return;
        }
        auto [t, flag] = stk.top(); stk.pop();
        auto [begin, next] = t;

        if (node == nullptr) {
            add_jump(begin, TOK_EPSILON, next);
        } else if (node->isType(ExprType::T_OR)) {
            auto branch = static_cast<Or*>(node);
            size_t i = branch->items.size();
            while (i--) {
                stk.push(new_pair(begin, next));
            }
        } else if (node->isType(ExprType::T_SEQUENCE)) {
            auto seq = static_cast<Sequence*>(node);
            size_t k = seq->nodes.size();
            if (flag) { // Or mode in Class
                while (k--) {
                    stk.push(new_pair(begin, next, flag));
                }
            } else {
                State s = next;
                for (size_t i=0; i<k-1; i++) {
                    State t = new_state();
                    stk.push(new_pair(t, s));
                    s = t;
                }
                stk.push(new_pair(begin, s));
            }
        } else if (node->isType(ExprType::T_QUANTIFIER)) {
            auto q = static_cast<Quantifier*>(node);
            if (q->max == 0) return;
            State s;
            if (q->min == q->max) {
                int m = q->min - 1;
                while (m-- > 0) {
                    s = new_state();
                    stk.push(new_pair(begin, s));
                    q->prev->travel(fn);
                    begin = s;
                }
                stk.push(new_pair(begin, next));
            } else {
                int m = q->min;
                while (m-- > 0) {
                    s = new_state();
                    stk.push(new_pair(begin, s));
                    q->prev->travel(fn);
                    begin = s;
                }
                add_jump(begin, TOK_EPSILON, next);
                if (q->max == INF) {
                    stk.push(new_pair(next, next));
                } else {
                    for (m = q->min + 1; m < q->max; m++) {
                        s = new_state();
                        add_jump(s, TOK_EPSILON, next);
                        stk.push(new_pair(begin, s));
                        q->prev->travel(fn);
                        begin = s;
                    }
                    stk.push(new_pair(begin, next));
                }
            }
        } else if (node->isType(ExprType::T_CLASS)) {
            auto cls = static_cast<Class*>(node);
            if (cls->negative) {
                throw std::runtime_error("NFA not support negative class!");
            }
            stk.push(new_pair(begin, next, 1));
        } else if (node->isType(ExprType::T_RANGE)) {
            auto range = static_cast<Range*>(node);
            assert(range->start.size() == 1);
            assert(range->end.size() == 1);

            char a = range->start[0];
            char b = range->end[0];
            for (char c=a; c<=b; c++) {
                Token t = get_token({c});
                add_jump(begin, t, next);
            }
        } else if (node->isType(ExprType::T_GROUP)) {
            // auto group = static_cast<Group*>(node);
            stk.push(new_pair(begin, next));
        } else if (node->isType(ExprType::T_LITERAL)) {
            auto literal = static_cast<Literal*>(node);
            for (size_t i=0; i<literal->escaped.size();) {
                int len = (literal->escaped[i] == '\\') ? 2 : 1;
                std::string tok = literal->escaped.substr(i, len);
                Token t = get_token(tok);
                i += len;
                if (flag) {
                    add_jump(begin, t, next);
                } else {
                    State s = i < literal->escaped.size()? new_state() : next;
                    add_jump(begin, t, s);
                    begin = s;
                }
            }
        } else if (node->isType(ExprType::T_ANCHOR)) {
            auto anchor = static_cast<Anchor*>(node);
            Token t = get_token(special_token(anchor->val));
            add_jump(begin, t, next);
        } else if (node->isType(ExprType::T_ANY)) {
            Token t = get_token(special_token("."));
            add_jump(begin, t, next);
        } else if (node->isType(ExprType::T_ESCAPED)) {
            auto escaped = static_cast<Escaped*>(node);
            Token t;
            if (utf8_encoding && escaped->isUnicode()) {
                t = get_token(uhhhh_to_utf8(escaped->ch));
            } else {
                t = get_token(special_token(escaped->ch));
            }
            add_jump(begin, t, next);
        } else {
            // not supported
            std::string err = "NFA not support for: " + node->typeName();
            DEBUG_OS << err << "\n";
            throw std::runtime_error(err);
        }

    };

    expr->travel(fn);
}

Bits DFA::find_closure(const Bits& b, Token t) {
    Bits res(b.size(), false);
    for (State s=0; s<b.size(); s++) {
        if (!b[s]) continue;
        auto it = nfa->nfa[s].find(t);
        if (it == nfa->nfa[s].end()) continue;
        for (State next : it->second) {
            res[next] = true;
            for (State x : epsClosure[next]) {
                res[x] = true;
            }
        }
    }
    return res;
}

void DFA::add_jump(State a, Token t, State b) {
    while (dfa.size() <= std::max(a, b)) dfa.push_back({});
    auto it = dfa[a].find(t);
    if (it != dfa[a].end()) {
        assert(it->second == b);
    }
    dfa[a][t] = b;
}

void DFA::dump(std::ostream& os) {

    const std::vector<std::string>& tokens = nfa->tokens;
    size_t max_tok_len = std::to_string(dfa.size()).size();
    for (const std::string& t : tokens) {
        if (t == EPSILON) continue;
        max_tok_len = std::max(max_tok_len, visual_width(t));
    }
    size_t width = max_tok_len + 1;

    std::string border(std::min((width * tokens.size())/2, (size_t)40), '=');
    os << "\n" << border << " DFA Start " << border << "\n";

    os << std::setw(10) << std::right << "Tokens:";
    for (const std::string& t : tokens) {
        if (t == EPSILON) continue;
        size_t w = t.size()-visual_width(t);
        os << std::setw(width + w) << std::right << t;
    }
    os << "\n";

    for (State s=0; s < dfa.size(); s++) {
        if (!is_valid_state(s)) continue;;
        std::string ss = std::to_string(s);
        if (s == 0) {
            ss += ">";
        }
        if (terminals.find(s) != terminals.end()) {
            ss += "*";
        }
        os << std::setw(10) << std::right << ("State " + ss + ":");

        auto& mp = dfa[s];
        for (Token tok = 0; tok < tokens.size(); tok++) {
            if (tok == TOK_EPSILON) continue;
            auto it = mp.find(tok);
            if (it != mp.end()) {
                os << std::setw(width) << std::right << it->second;
            } else {
                os << std::setw(width) << std::right << "-";
            }
        }
        os << "\n";
    }

    os << "Terminal: ";
    bool first = true;
    for (State s : terminals) {
        if (!first) os << ", ";
        os << s;
        first = false;
    }
    os << "\n";
    os << border << "  DFA End  " << border << "\n";
}

void DFA::epsilon_closure(State x, State s) {
    std::unordered_set<State>& closure = epsClosure[x];
    if (closure.find(s) != closure.end()) return;
    closure.insert(s);
    if (s < x) {
        for (State next : epsClosure[s]) {
            closure.insert(next);
        }
        return;
    }
    auto it = nfa->nfa[s].find(TOK_EPSILON);
    if (it != nfa->nfa[s].end()) {
        for (State next : it->second) {
            epsilon_closure(x, next);
        }
    }
};

void DFA::nfa_to_dfa() {
    auto findClosureId = [this](const Bits& bs) {
        for (State s=0; s<nfa_closure.size(); s++) {
            if (bs == nfa_closure[s]) {
                return s;
            }
        }
        return nfa_closure.size();
    };

    State s = 0;
    while (s < nfa_closure.size()) {
        for (Token tok = TOK_EPSILON+1; tok < nfa->tokens.size(); tok++) {
            Bits r = find_closure(nfa_closure[s], tok);
            State next = findClosureId(r);
            if (next == nfa_closure.size()) {
                nfa_closure.push_back(r);
            }
            add_jump(s, tok, next);
            if (r[nfa->state_final]) {
                terminals.insert(next);
            }
        }
        s++;
    }
}

bool DFA::is_valid_state(State s) {
    return valids.find(s) != valids.end();
}

void DFA::simplify() {
    auto is_terminal = [this](State s) {
        return terminals.find(s) != terminals.end();
    };

    std::vector<bool> visited(dfa.size(), false);
    std::function<bool(State)> dfs = [&is_terminal,&dfs,&visited,this](State s) {
        if (is_valid_state(s) || is_terminal(s)) return true;
        if (visited[s]) return false;
        visited[s] = true;
        for (auto it = this->dfa[s].begin(); it != this->dfa[s].end(); ++it) {
            State next = it->second;
            if (dfs(next)) {
                valids.insert(s);
                return true;
            }
        }
        return false;
    };

    // 查找无效节点
    for (State s = 0; s < dfa.size(); s++) {
        if (dfs(s)) valids.insert(s);
    }

    // 合并等价节点
    std::vector<int> fa(dfa.size(), -1);
    for (State x = 0; x < dfa.size(); x++) {
        for (State y = x+1; y < dfa.size(); y++) {
            if ((is_terminal(x) && is_terminal(y)) || (!is_terminal(x) && !is_terminal(y))) {
                if (nfa_closure[x] == nfa_closure[y]) {
                    fa[y] = fa[x]<0? x : fa[x];
                }
            }
        }
    }

    for (State s = 0; s < dfa.size(); s++) {
        auto& mp = dfa[s];
        if (!is_valid_state(s)) {
            mp.clear();
            continue;
        }
        for (auto it = mp.begin(); it != mp.end();) {
            State next = it->second;
            if (!is_valid_state(next)) {
                it = mp.erase(it);
                continue;;
            } else if (fa[next] >= 0) {
                it->second = fa[next];
            }
            ++it;
        }
    }
}

void DFA::generate() {
    size_t states = nfa->nfa.size();
    for (State s = 0; s < states; s++) {
        epsClosure.push_back({});
        epsilon_closure(s, s);
    }
    
    nfa_closure.emplace_back(states, false);

    // initial state
    for (auto s : epsClosure[0]) {
        nfa_closure[0][s] = true;
    }

    nfa_to_dfa();

    simplify();
}
