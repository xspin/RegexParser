#include <iostream>
#include <cmath>
#include "unicode.h"
#include "DFA.h"
#include "GraphBox.h"
#include <numeric>
#include <queue>

using Pos = std::pair<int,int>;
#define BASE 10000

static std::string special_token(const std::string& s) {
    return "(" + s + ")";
}

Token NFA::tok_epsilon = 0;
State NFA::state_initial = 0;
State NFA::state_final = 1;

NFA::NFA(bool color): color(color) {
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
        ss = ("State " + ss + ":");
        if (s == state_initial) ss = ">" + ss;
        else if (s == state_final) ss = "*" + ss;
        if (color && (s == state_initial || s == state_final)) {
            ss = iter_color_pack(ss);
        }
        os << visual_str_pad(ss, w, Align::RIGHT) << "\n";
        for (auto it = nfa[s].begin(); it != nfa[s].end(); ++it) {
            Token t = it->first;
            std::string tok;
            if (t == TOK_EPSILON) {
                tok = iter_color_pack(tokens[t]);
            } else {
                tok = tokens[t];
            }
            // int d = tokens[t].size() - visual_width(tokens[t]);
            // os << std::setw(w+d-2) << std::right << (tokens[t] + ":") << " ";
            os << visual_str_pad(tok, w-1, Align::RIGHT) << ": ";
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

    enum class Flag {
        Default,
        InClass,
        Skip
    };

    using Sitem = std::pair<std::pair<State,State>,Flag>;
    std::stack<Sitem> stk; // begin state, next state, flag

    auto make_item = [](State a, State b, Flag flag=Flag::Default) -> Sitem {
        return std::make_pair(std::make_pair(a, b), flag);
    };

    stk.push(make_item(start, end));

    std::function<void(ExprNode*)> fn = [&](ExprNode* node) {
        if (stk.empty()) {
            // DEBUG_OS << "Empty stk! node " << (node?node->fmt(0,false) : "NULL") << "\n";
            return;
        }

        auto [t, flag] = stk.top(); stk.pop();
        auto [begin, next] = t;
        if (flag == Flag::Skip) return;

        if (node == nullptr) {
            add_jump(begin, TOK_EPSILON, next);
        } else if (node->isType(ExprType::T_OR)) {
            auto branch = static_cast<Or*>(node);
            size_t i = branch->items.size();
            while (i--) {
                stk.push(make_item(begin, next));
            }
        } else if (node->isType(ExprType::T_SEQUENCE)) {
            auto seq = static_cast<Sequence*>(node);
            size_t k = seq->nodes.size();
            if (flag == Flag::InClass) { // Or mode in Class
                while (k--) {
                    stk.push(make_item(begin, next, flag));
                }
            } else {
                State s = next;
                for (size_t i=0; i<k-1; i++) {
                    State t = new_state();
                    stk.push(make_item(t, s));
                    s = t;
                }
                stk.push(make_item(begin, s));
            }
        } else if (node->isType(ExprType::T_QUANTIFIER)) {
            auto q = static_cast<Quantifier*>(node);
            State s;
            if (q->max == 0) {
                add_jump(begin, TOK_EPSILON, next);
                stk.push(make_item(begin, next, Flag::Skip));
                return;
            }
            if (q->min == q->max) {
                int m = q->min - 1;
                while (m-- > 0) {
                    s = new_state();
                    stk.push(make_item(begin, s));
                    q->prev->travel(fn);
                    begin = s;
                }
                stk.push(make_item(begin, next));
            } else {
                int m = q->min;
                while (m-- > 0) {
                    s = new_state();
                    stk.push(make_item(begin, s));
                    q->prev->travel(fn);
                    begin = s;
                }
                add_jump(begin, TOK_EPSILON, next);
                if (q->max == INF) {
                    stk.push(make_item(next, next));
                } else {
                    for (m = q->min + 1; m < q->max; m++) {
                        s = new_state();
                        add_jump(s, TOK_EPSILON, next);
                        stk.push(make_item(begin, s));
                        q->prev->travel(fn);
                        begin = s;
                    }
                    stk.push(make_item(begin, next));
                }
            }
        } else if (node->isType(ExprType::T_CLASS)) {
            auto cls = static_cast<Class*>(node);
            if (cls->negative) {
                throw std::runtime_error("DFA not support negative class!");
            }
            stk.push(make_item(begin, next, Flag::InClass));
        } else if (node->isType(ExprType::T_RANGE)) {
            auto range = static_cast<Range*>(node);
            if (range->start.size() == 1 && range->end.size() == 1) {
                char a = range->start[0];
                char b = range->end[0];
                if (b - a < 30) {
                    for (char c=a; c<=b; c++) {
                        Token t = get_token({c});
                        add_jump(begin, t, next);
                    }
                } else {
                    // keep it if range too large
                    Token t = get_token(range->start + "-" + range->end);
                    add_jump(begin, t, next);
                }
            } else {
                Token t = get_token(range->start + "-" + range->end);
                add_jump(begin, t, next);
            }
        } else if (node->isType(ExprType::T_GROUP)) {
            // auto group = static_cast<Group*>(node);
            stk.push(make_item(begin, next));
        } else if (node->isType(ExprType::T_LITERAL)) {
            auto literal = static_cast<Literal*>(node);
            for (size_t i=0; i<literal->escaped.size();) {
                int len = (literal->escaped[i] == '\\') ? 2 : 1;
                std::string tok = literal->escaped.substr(i, len);
                Token t = get_token(tok);
                i += len;
                if (flag == Flag::InClass) {
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
            if (escaped->isUnicode()) {
                if (utf8_encoding) {
                    t = get_token(uhhhh_to_utf8(escaped->ch));
                } else {
                    auto tmp = "U+" + escaped->ch.substr(2);
                    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
                    t = get_token(tmp);
                }
            } else if (escaped->isHex()) {
                auto tmp = "0x" + escaped->ch.substr(2);
                std::transform(tmp.begin()+2, tmp.end(), tmp.begin()+2, ::toupper);
                t = get_token(tmp);
            } else {
                t = get_token(special_token(escaped->ch));
            }
            add_jump(begin, t, next);
        } else {
            // not supported
            std::string err = "DFA not support for: " + node->typeName();
            DEBUG_OS << err << "\n";
            throw std::runtime_error(err);
        }

    };

    expr->travel(fn);
}

void NFA::simplify()
{
    return;
    std::vector<int> eps_next(nfa.size(), -1);
    for (State s = 0; s < nfa.size(); s++) {
        auto& mp = nfa[s];
        if (mp.size() == 1 && mp.find(TOK_EPSILON) != mp.end()) {
            eps_next[s] = *mp[TOK_EPSILON].begin();
        }
    }
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
    auto pack_color = [this](const std::string& s) {
        return this->nfa->color ? iter_color_pack(s) : s;
    };

    const std::vector<std::string>& tokens = nfa->tokens;
    size_t max_tok_len = std::to_string(dfa.size()).size();
    for (const std::string& t : tokens) {
        if (t == EPSILON) continue;
        max_tok_len = std::max(max_tok_len, visual_width(t));
    }
    size_t width = max_tok_len + 1;

    size_t bw = std::max((size_t)10, std::min((width * tokens.size())/2, (size_t)40));
    std::string border(bw, '=');
    os << "\n" << border << " DFA Start " << border << "\n";

    size_t header_len = 13;
    os << std::setw(header_len) << std::right << "Tokens:";
    for (const std::string& t : tokens) {
        if (t == EPSILON) continue;
        auto tk = pack_color(t);
        os << visual_str_pad(tk, width, Align::RIGHT);
    }
    os << "\n";

    for (State s=0; s < dfa.size(); s++) {
        if (!is_valid(s)) continue;;
        std::string ss = "State " + std::to_string(s) + ":";
        bool c = false;
        if (s == 0) {
            ss = ">" + ss;
            c = true;
        }
        if (terminals.find(s) != terminals.end()) {
            ss = "*" + ss;
            c = true;
        }
        if (c) ss = pack_color(ss);
        os << visual_str_pad(ss, header_len, Align::RIGHT);

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

    os << "Accept States: ";
    bool first = true;
    for (State s : terminals) {
        if (!is_valid(s)) continue;
        if (!first) os << ", ";
        os << s;
        first = false;
    }
    os << "\n";
    os << border << "  DFA End  " << border << "\n";
}

bool DFA::is_color() {
    return nfa->color;
}



void DFA::nfa_to_dfa() {
    std::vector<std::unordered_set<State>> epsClosure;

    std::function<void(State,State)> epsilon_closure = [&epsilon_closure,this,&epsClosure](State x, State s) {
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

    auto find_closure = [this,&epsClosure](const Bits& b, Token t) {
        Bits res(b.size(), 0);
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
    };

    size_t states = nfa->nfa.size();
    for (State s = 0; s < states; s++) {
        epsClosure.push_back({});
        epsilon_closure(s, s);
    }

    std::vector<Bits> nfa_closure; // [dfa state: {nfa states closure...}, ...]
    
    nfa_closure.emplace_back(states, false);

    // initial state
    for (auto s : epsClosure[0]) {
        nfa_closure[0][s] = true;
    }

    auto findClosureId = [](const std::vector<Bits>& closures, const Bits& bs) {
        for (State s = 0; s < closures.size(); s++) {
            if (bs == closures[s]) {
                return s;
            }
        }
        return closures.size();
    };

    State s = 0;
    while (s < nfa_closure.size()) {
        for (Token tok = TOK_EPSILON+1; tok < nfa->tokens.size(); tok++) {
            Bits r = find_closure(nfa_closure[s], tok);
            State next = findClosureId(nfa_closure, r);
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

bool DFA::is_valid(State s) {
    return s!=INVALID_STATE && valids.find(s) != valids.end();
}

bool DFA::is_accepted(State s) {
    return terminals.find(s) != terminals.end();
}

State DFA::next(State s, Token t) {
    auto& mp = dfa[s];
    auto it = mp.find(t);
    if (it == mp.end()) {
        return INVALID_TOKEN;
    }
    return it->second;
}

const std::unordered_map<Token,State>* DFA::next(State s) {
    return &dfa[s];
}

size_t DFA::states() {
    return dfa.size();
}

std::string DFA::token_name(Token t) {
    std::string r = t < nfa->tokens.size() ? nfa->tokens[t] : "Invalid";
    return r;
}

const std::vector<std::string> DFA::get_tokens() {
    return nfa->tokens;
}

void DFA::simplify() {
    std::vector<bool> visited(dfa.size(), false);

    std::function<void(State)> dfs = [&dfs,&visited,this](State s) {
        if (visited[s]) return;
        visited[s] = true;
        if (is_accepted(s)) {
            valids.insert(s);
        }
        for (auto [tok, next] : dfa[s]) {
            dfs(next);
            if (valids.count(next)) {
                valids.insert(s);
            }
        }
    };

    // 查找无效节点
    dfs(state_initial);

    std::vector<State> a, b;
    for (State s = 0; s < dfa.size(); s++) {
        if (!is_valid(s)) continue;
        if (is_accepted(s)) {
            a.push_back(s);
        } else {
            b.push_back(s);
        }
    }
    std::unordered_map<State,size_t> setid;
    setid[INVALID_STATE] = INVALID_STATE;
    for (State s : a) {
        setid[s] = a[0];
    }
    for (State s : b) {
        setid[s] = b[0];
    }
    std::queue<std::vector<State>> equiv_sets;
    equiv_sets.push(a);
    equiv_sets.push(b);

    std::vector<std::vector<State>> nexts(dfa.size(), std::vector<State>(nfa->tokens.size(), INVALID_STATE));
    for (State s : valids) {
        for (auto [tok, next] : dfa[s]) {
            nexts[s][tok] = next;
        }
    };
    auto next_equal = [this, &nexts, &setid](State a, State b) {
        for (size_t i = 0; i < nfa->tokens.size(); i++) {
            if (setid[nexts[a][i]] != setid[nexts[b][i]]) return false;
        }
        return true;
    };

    auto find_equiv = [&](std::vector<std::pair<State,std::vector<State>>>&mp, State s) {
        for (auto& [x, p] : mp) {
            if (next_equal(s, x)) {
                p.push_back(s);
                return;
            }
        }
        mp.emplace_back(s, std::vector<State>{s});
    };

    while (!equiv_sets.empty()) {
        auto vec = equiv_sets.front();
        equiv_sets.pop();
        if (vec.size() <= 1) continue;

        std::vector<std::pair<State,std::vector<State>>> mp;
        for (State s : vec) {
            find_equiv(mp, s);
        }
        if (mp.size() > 1) {
            for (auto& [x, xs] : mp) {
                equiv_sets.push(xs);
                for (auto y : xs) setid[y] = x;
            }
        }
    }

    for (State s = 0; s < dfa.size(); s++) {
        auto& mp = dfa[s];
        if (setid[s] != s) {
            dfa[s].clear();
            continue;
        }
        for (auto it = mp.begin(); it != mp.end();) {
            State next = it->second;
            if (!is_valid(next)) {
                it = mp.erase(it);
                continue;;
            } else {
                it->second = setid[next];
            }
            ++it;
        }
    }
    for (State s = 0; s < dfa.size(); s++) {
        if (setid[s] != s) {
            valids.erase(s);
        }
    }
}

void DFA::generate() {

    nfa_to_dfa();

    simplify();
}

#if 0
DFAGraph::DFAGraph(DFA* dfa): dfa(dfa) {
    assert(dfa);
}

// keep "" for unicode chars
static inline bool str_empty(const std::string& s) {
    return s == " ";
};

static void draw_vertical_line(std::string& org) {
    if (org == Line::HORIZON) {
        org = Line::CROSS;
    } else if (str_empty(org)) {
        org = Line::VERTICAL;
    }
}

static void draw_horizon_line(std::string& org) {
    if (org == Line::VERTICAL) {
        org = Line::CROSS;
    } else if (str_empty(org)) {
        org = Line::HORIZON;
    }
}

void DFAGraph::draw_links(Block& canvas, 
    std::vector<Pos> &positions, 
    std::pair<size_t,size_t> width_height, 
    std::pair<size_t,size_t> line_size) {
    
    size_t block_width = width_height.first;
    size_t block_height = width_height.second;
    size_t row_line_size = line_size.first;
    size_t col_line_size = line_size.second;
    
    auto get_left_up_pos = [&](State s) -> Pos {
        return positions[s];
    };
    auto get_left_down_pos = [&](State s) -> Pos {
        auto [i, j] = positions[s];
        return std::make_pair(i + block_height-1, j);
    };
    auto get_right_up_pos = [&](State s) -> Pos {
        auto [i, j] = positions[s];
        return std::make_pair(i, j + block_width-1);
    };
    auto get_right_down_pos = [&](State s) -> Pos {
        auto [i, j] = positions[s];
        return std::make_pair(i + block_height-1, j + block_width-1);
    };
    auto get_right_center_pos = [&](State s) -> Pos {
        auto [i, j] = positions[s];
        return std::make_pair(i + block_height/2, j + block_width-1);
    };

    auto draw_hline = [&](int i, int aj, int bj) {
        for (int t = aj; t <= bj; t++) {
            draw_horizon_line(canvas[i][t]);
        }
    };
    auto draw_vline = [&](int j, int ai, int bi) {
        for (int t = ai; t <= bi; t++) {
            draw_vertical_line(canvas[t][j]);
        }
    };

    auto set_str = [&canvas, this](int i, int j, std::string s, bool right) {
        if (dfa->is_color()) {
            s = iter_color_pack(s);
        }
        if (!str_empty(canvas[i][j])) {
            s = canvas[i][j] + "|" + s;
        }
        int k = visual_len(s);
        if (k > 1) {
            std::string empty = "";
            int l;
            l = right ? j :  j-k+1;
            assert (l >= 0);
            for (int t = l; t < l+k; t++) {
                canvas[i][t] = empty;
            }
        }
        canvas[i][j] = s;
    };

    auto link_right_to = [&](State a, State b, Token t) {
        auto [ai, aj] = positions[a];
        auto [bi, bj] = positions[b];
        assert(ai == bi && aj < bj);
        auto [i, j] = get_right_center_pos(a);
        draw_hline(i, j+1, bj-1);
        canvas[i][bj-1] = ">";
        // canvas[i-1][j+1] = dfa->token_name(t); 
        set_str(i-1, j+1, dfa->token_name(t), true);
    };

    auto get_row_increase = [&](int i, State s) {
        return (i + 1) + s * row_line_size + row_line_size/2;
    };

    auto get_row_decrease = [&](int i, State s) {
        //  i - 1 - (dfa->states() - s - 1) * row_line_size - row_line_size/2; // line_size is odd
        return (i - row_line_size * dfa->states()) + s * row_line_size + row_line_size/2;
    };

    auto get_col_increase = [&](int j, State s) {
        return j + (1 + s) * col_line_size - 1;
    };
/*
   ⭣ ⭡ 
 ⭢[ x ]⭢
   ⭡ ⭣
*/
    auto link_vertical = [&](State a, State b) { // a --> b
        auto [ai, aj] = positions[a];
        auto [bi, bj] = positions[b];
        Pos aa, bb;
/*
[ a ]
   ⭣
⭣  
[ b ]
*/
        if (ai < bi) {
            auto [xi, xj] = get_right_down_pos(a);
            int r = get_row_increase(xi, b);
            xj--;
            draw_vline(xj, xi + 1, r);
            aa = {r, xj};

            auto [yi, yj] = get_left_up_pos(b);
            r = get_row_decrease(yi, b);
            yj++;
            draw_vline(yj, r, yi - 1);
            bb = {r, yj};

        } else if (ai > bi) {
/*
[ b ]
⭡  
   ⭡
[ a ]
*/
            auto [xi, xj] = get_right_up_pos(a);
            int r = get_row_decrease(xi, b);
            xj--;
            draw_vline(xj, r, xi - 1);
            aa = {r, xj};

            auto [yi, yj] = get_left_down_pos(b);
            r = get_row_increase(yi, b);
            yj++;
            draw_vline(yj, yi + 1, r);
            bb = {r, yj};
        } else { // same raw
/*
⭣        ⭡
[ b ]  [ a ]
*/
            auto [xi, xj] = get_right_up_pos(a);
            int r = get_row_decrease(xi, b);
            xj--;
            draw_vline(xj, r, xi - 1);
            aa = {r, xj};


            auto [yi, yj] = get_left_up_pos(b);
            r = get_row_decrease(yi, b);
            yj++;
            draw_vline(yj, r, yi - 1);
            bb = {r, yj};
        }
        return std::make_pair(aa, bb);
    };

    auto link_horizon = [&](State a, State b, Pos aa, Pos bb, Token t) { // a --> b
        auto [xi, xj] = aa;
        auto [yi, yj] = bb;

        if (xi == yi) { // neighbor row
            if (xj < yj) {
                draw_hline(xi, xj, yj);
                // canvas[xi-1][xj+1] = dfa->token_name(t);
                canvas[xi][xj+1] = ">";
                set_str(xi-1, xj+1, dfa->token_name(t), true);
            } else {
                draw_hline(xi, yj, xj);
                // canvas[xi-1][xj-1] = dfa->token_name(t); 
                canvas[xi][xj-1] = "<";
                set_str(xi-1, xj-1, dfa->token_name(t), false);
            }
        } else {
            int c = get_col_increase(xj, b);
            draw_hline(xi, xj, c);
            // canvas[xi-1][xj+1] = dfa->token_name(t);
            canvas[xi][xj+1] = ">";
            set_str(xi-1, xj+1, dfa->token_name(t), true);
            if (yj < c) {
                draw_hline(yi, yj, c);
            } else {
                draw_hline(yi, c, yj);
            }

            if (xi < yi) {
                draw_vline(c, xi, yi);
            } else {
                draw_vline(c, yi, xi);
            }
        }
    };

    auto is_neighbor = [&](State a, State b) {
        auto [ai, aj] = positions[a];
        auto [bi, bj] = positions[b];
        return (ai == bi && std::abs(aj-bj) <= dfa->states()*col_line_size + block_width);
    };

    auto link_to = [&](State a, State b, Token t) {
        auto [aa, bb] = link_vertical(a, b);
        link_horizon(a, b, aa, bb, t);
    };

    auto link = [&](State a, State b, Token t) { // a --> b
        if (!dfa->is_valid(a) || !dfa->is_valid(b)) return;
        auto [ai, aj] = positions[a];
        auto [bi, bj] = positions[b];
        assert(ai + aj > 0 && bi + bj > 0);
        if (a!=b && is_neighbor(a, b) && aj < bj) {
            link_right_to(a, b, t);
        } else {
            link_to(a, b, t);
        }
    };


    for (State s = 0; s < dfa->states(); s++) {
        auto mp = dfa->next(s);
        for (auto it = mp->begin(); it != mp->end(); ++it) {
            link(s, it->second, it->first);
        }
    }

}

std::vector<std::vector<State>> DFAGraph::find_paths() {

    std::vector<bool> visited(dfa->states(), false);

    std::function<bool(State,std::vector<State>&,bool)> find_path = 
    [&find_path, &visited, this](State s,std::vector<State>&path, bool visited_ret) {
        if (!dfa->is_valid(s)) return false;
        if (visited[s]) return visited_ret;
        visited[s] = true;
        path.push_back(s);
        for (auto [tok, next] : *dfa->next(s)) {
            if (find_path(next, path, visited_ret)) return true;
        }
        if (dfa->is_accepted(s)) {
            return true;
        }
        path.pop_back();
        visited[s] = false;
        return false;
    };

    size_t len = std::sqrt(dfa->states()) + 1;

    std::vector<std::vector<State>> paths = {{}};

    find_path(dfa->state_initial, paths.front(), false);

    len = std::max(len, paths.front().size());

    auto find_append_path = [&](State s) {
        std::vector<State> path;
        if (find_path(s, path, true) && !path.empty()) {
            if (paths.size() >= 2 && paths.back().size() + path.size() <= len) {
                for (auto v : path) {
                    paths.back().push_back(v);
                }
            } else {
                paths.push_back(path);
            }
        }
    };


    for (State s : paths.front()) {
        for (auto [tok, next] : *dfa->next(s)) {
            find_append_path(next);
        }
    }

    for (State s : dfa->valids) {
        if (!visited[s]) {
            find_append_path(s);
        }
    }

    // std::sort(paths.begin()+1, paths.end(), [](std::vector<State>&a, std::vector<State>&b) {
    //     if (a.size() != b.size()) return a.size() > b.size();
    //     return a[0] < b[0];
    // });

    return paths;
}

void DFAGraph::to_rows(size_t block_width, size_t block_height,
    const std::vector<Pos>& position) {

    auto is_not_line = [this](int i, int j) {
        if (i < 0 || i >= canvas.size() || j < 0 || j >= canvas[0].size()) return true;
        std::string s = canvas[i][j];
        return !(s == Line::HORIZON || s == Line::VERTICAL || s == Line::CROSS || s == "<" || s == ">");
    };

    for (int i = 0; i < canvas.size(); i++) {
        for (int j = 0; j < canvas[i].size(); ++j) {
            if (canvas[i][j] != Line::CROSS) continue;
            int up = is_not_line(i-1, j);
            int down = is_not_line(i+1, j);
            int left = is_not_line(i, j-1);
            int right = is_not_line(i, j+1);
            int c = up + down + left + right; 
            if (c == 1) {
                if (right) canvas[i][j] = Line::RIGHT_T;
                else if (left) canvas[i][j] = Line::LEFT_T;
                else if (up)  canvas[i][j] = Line::UP_T;
                else if (down)  canvas[i][j] = Line::DOWN_T;
            } else if (c == 2) {
                if (left && up) canvas[i][j] = Line::LEFT_TOP;
                else if (right && up) canvas[i][j] = Line::RIGHT_TOP;
                else if (left && down) canvas[i][j] = Line::LEFT_BOTTOM;
                else if (right && down) canvas[i][j] = Line::RIGHT_BOTTOM;
            }
        }
    }

    std::unordered_set<size_t> skip_rows, skip_cols;
    for (State s = 0; s < position.size(); s++) {
        auto [i, j] = position[s];
        if (i + j == 0) continue;
        for (size_t row = std::max(0, i-1); row <= i + block_height; row++) {
            skip_rows.insert(row);
        }
        for (size_t col = std::max(j-2, 0); col <= j+1 + block_width; col++) {
            skip_cols.insert(col);
        }
        int ui = i;
        int uj = j+1;
        if (!str_empty(canvas[ui-1][uj])) {
            canvas[ui-1][uj] = Line::ARROW_DOWN;
            canvas[ui][uj] = dfa->is_accepted(s) ? 
                get_table_line(TableId::DOUBLE, TableLine::TAB_DOWN_T) : Line::DOWN_T;
        }
        uj = j + block_width - 2;
        if (!str_empty(canvas[ui-1][uj])) {
            // canvas[ui-1][uj] = Line::ARROW_UP;
            canvas[ui][uj] = dfa->is_accepted(s) ? 
                get_table_line(TableId::DOUBLE, TableLine::TAB_DOWN_T) : Line::DOWN_T;
        }

        int di = i + block_height - 1;
        int dj = j+1;
        if (!str_empty(canvas[di+1][dj])) {
            canvas[di+1][dj] = Line::ARROW_UP;
            canvas[di][dj] = dfa->is_accepted(s) ? 
                get_table_line(TableId::DOUBLE, TableLine::TAB_UP_T) : Line::UP_T;
        }
        dj = j + block_width - 2;
        if (!str_empty(canvas[di+1][dj])) {
            canvas[di][dj] = dfa->is_accepted(s) ? 
                get_table_line(TableId::DOUBLE, TableLine::TAB_UP_T) : Line::UP_T;
        }
    }


    std::unordered_set<size_t> omit_row, omit_col;
    for (size_t i = 0; i < canvas.size(); i++) {
        if (skip_rows.count(i)) continue;
        bool vline = false;
        bool hline = false;
        for (size_t j = 0; j < canvas[i].size(); ++j) {
            if (str_empty(canvas[i][j])) continue;
            if (canvas[i][j] == Line::HORIZON) {
                hline = true;
            } else if (canvas[i][j] == Line::VERTICAL) {
                vline = true;
            } else {
                vline = true;
                hline = true;
            }
            if (vline && hline) break;
        }
        if (!(vline && hline)) {
            omit_row.insert(i);
        }
    }

    for (size_t j = 0; j < canvas[0].size(); ++j) {
        if (skip_cols.count(j)) continue;
        bool vline = false;
        bool hline = false;
        for (size_t i = 0; i < canvas.size(); i++) {
            if (str_empty(canvas[i][j])) continue;
            if (canvas[i][j] == Line::HORIZON) {
                hline = true;
            } else if (canvas[i][j] == Line::VERTICAL) {
                vline = true;
            } else {
                vline = true;
                hline = true;
            }
            if (vline && hline) break;
        }
        if (!(vline && hline)) {
            omit_col.insert(j);
        }
    }

    for (size_t i = 0; i < canvas.size(); i++) {
        std::stringstream ss;
        if (omit_row.count(i)) continue;
        for (size_t j = 0; j < canvas[i].size(); ++j) {
            if (omit_col.count(j)) continue;
            ss << canvas[i][j];
        }
        rows.push_back(ss.str());
    }
}


std::vector<Pos> DFAGraph::init_canvas(const std::vector<std::vector<State>> &paths,
    std::pair<size_t,size_t> wxh, std::pair<size_t,size_t> line_size) {

    auto [block_width, block_height] = wxh;
    auto [row_line_size, col_line_size] = line_size;

    size_t max_blocks = std::accumulate(paths.begin(), paths.end(), (size_t)0, [](size_t k, const auto& b) {
        return std::max(k, b.size());
    });

    size_t row_block_spaces = dfa->states() * row_line_size;
    size_t col_block_spaces = dfa->states() * col_line_size;

    size_t cols = max_blocks * (block_width + col_block_spaces) + col_block_spaces;
    size_t rows = paths.size() * (block_height + row_block_spaces) + row_block_spaces;


    canvas = std::vector<std::vector<std::string>>(rows, std::vector<std::string>(cols, " "));

    size_t i = 0;
    size_t j = 0;
    std::vector<Pos> positions(dfa->states());
    i = row_block_spaces;
    for (auto& path : paths) {
        j = col_block_spaces;
        for (State s : path) {
            TableId id = dfa->is_accepted(s) ? TableId::DOUBLE : TableId::ROUND;
            auto b = build_block(block_width, block_height, std::to_string(s), id);
            if (dfa->is_color() && (s == dfa->state_initial || dfa->is_accepted(s))) {
                auto color = iter_color();
                for (size_t r=0; r<b.size(); r++) {
                    b[r].front() = color + b[r].front();
                    b[r].back() += end_color();
                }
            }

            for (size_t x = 0; x < block_height; x++) {
                for (size_t y = 0; y < block_width; y++) {
                    canvas[i+x][j+y] = b[x][y];
                }
            }

            positions[s] = {i, j};
            j += block_width + col_block_spaces;
        }
        i += block_height + row_block_spaces;
    }

    return positions;
}


void DFAGraph::render() {

    std::vector<std::vector<State>> paths = find_paths();


    size_t token_len = 1;
    for (const std::string& tok : dfa->get_tokens()) {
        token_len = std::max(token_len, visual_len(tok));
    }

    // keep odd number
    size_t block_width = 5;
    size_t block_height = 3;
    size_t row_line_size = 5;
    size_t col_line_size = std::max(2*token_len + 1, (size_t)5);

    assert ((block_width&1) && (block_height&1) && (col_line_size&&1));

    canvas = {{}};

    std::vector<Pos> positions = init_canvas(paths, 
        {block_width, block_height}, {row_line_size, col_line_size});

    #if 0
    // std::vector<Pos> positions(dfa->states());
    for (auto& path : paths) {
        Block tmp;
        Block b = build_block(block_width, block_height, " ", TableId::EMPTY);
        tmp = block_concat(tmp, b, block_spaces, Dir::Horizon);
        for (State s : path) {
            TableId id = dfa->is_accepted(s) ? TableId::DOUBLE : TableId::ROUND;
            b = build_block(block_width, block_height, std::to_string(s), id);
            tmp = block_concat(tmp, b, block_spaces, Dir::Horizon);
        }
        b = build_block(block_width, block_height, " ", TableId::EMPTY);
        tmp = block_concat(tmp, b, block_spaces, Dir::Horizon);
        canvas = block_concat(canvas, tmp, block_spaces, Dir::Vertical); 

        int i = canvas.size() - block_height;
        int j = block_width + block_spaces;
        for (State s : path) {
            positions[s] = {i, j};
            // DEBUG_OS << s << " (" << i << ", " << j << ")\n";
            j += block_width + block_spaces;
        }
    }
    canvas = block_concat(canvas, {{}}, paths.back().size(), Dir::Vertical); 
    #endif

    draw_links(canvas, positions, {block_width, block_height}, {row_line_size, col_line_size});

    to_rows(block_width, block_height, positions);
}

void DFAGraph::dump(std::ostream& os) {
    for (const std::string& line : rows) {
        os << line << "\n";
    }
}
#endif