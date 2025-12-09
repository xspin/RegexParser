#include <unordered_map>
#include <numeric>
#include <cmath>
#include "DFACanvas.h"

#define BLOCK_WIDTH 7
#define BLOCK_HEIGHT 3
#define MIN_LINE_WIDTH 3
static size_t s_line_width = 3;

static inline size_t linkHash(State a, State b, Token t) {
    return (a<<20)|(b<<10)|t;
};

/*
   ⭣ ⭡ 
 ⭢[ x ]⭢
   ⭡ ⭣
*/

static inline size_t get_down_dist(size_t id) {
    return id * s_line_width + s_line_width/2;
}

static inline size_t get_up_dist(size_t id, size_t degrees) {
    return degrees * s_line_width +1 - get_down_dist(id);
}

static inline size_t get_right_dist(size_t id) {
    return 2 + id * s_line_width + s_line_width/2;
}

static inline Pos get_left_in_pos(const Pos& p) {
    auto [i, j] = p;
    return {i + BLOCK_HEIGHT/2, j-1};
}

static inline Pos get_right_out_pos(const Pos& p) {
    auto [i, j] = p;
    return {i + BLOCK_HEIGHT/2, j+BLOCK_WIDTH};
}


static inline std::pair<Pos,Pos> get_up_in_line(const Pos& p, size_t d) {
    auto [i, j] = p;
    return {{i-1, j+1}, {i-1-(d-1), j+1}};
}

static inline std::pair<Pos,Pos> get_up_out_line(const Pos& p, size_t d) {
    auto [i, j] = p;
    return {{i-1, j+BLOCK_WIDTH-2}, {i-1-(d-1), j+BLOCK_WIDTH-2}};
}

static inline std::pair<Pos,Pos> get_down_in_line(const Pos& p, size_t d) {
    auto [i, j] = p;
    return {{i+BLOCK_HEIGHT, j+1}, {i+BLOCK_HEIGHT+(d-1), j+1}};
}
static inline std::pair<Pos,Pos> get_down_out_line(const Pos& p, size_t d) {
    auto [i, j] = p;
    return {{i+BLOCK_HEIGHT, j+BLOCK_WIDTH-2}, {i+BLOCK_HEIGHT+(d-1), j+BLOCK_WIDTH-2}};
}

static inline bool str_empty(const std::string& s) {
    return s == " ";
}



DFACanvas::DFACanvas(DFA* dfa): dfa(dfa) {
    assert(dfa);
}

void DFACanvas::init(const std::vector<std::vector<State>> &paths) {
    size_t max_blocks = std::accumulate(paths.begin(), paths.end(), (size_t)0, [](size_t k, const auto& b) {
        return std::max(k, b.size());
    });

     size_t token_len = 1;
    for (const std::string& tok : dfa->get_tokens()) {
        token_len = std::max(token_len, visual_len(tok));
    }

    s_line_width =  std::max(2*token_len + 1, (size_t)5);

    
    degrees = 0;
    for (State s = 0; s < dfa->dfa.size(); s++) {
        for (auto& [tok, next] : dfa->dfa[s]) {
            linkid[linkHash(s, next, tok)] = degrees++;
        }
    }

    if (degrees > 50) s_line_width = MIN_LINE_WIDTH;

    size_t spaces = degrees * s_line_width;
    // DEBUG_OS << "spaces: " << spaces << "  degrees: " << degrees << " line width: " << s_line_width << "\n";
    size_t row_block_spaces = spaces;
    size_t col_block_spaces = spaces;

    size_t cols = max_blocks * (BLOCK_WIDTH + col_block_spaces) + col_block_spaces;
    size_t rows = paths.size() * (BLOCK_HEIGHT + row_block_spaces) + row_block_spaces;

    // this->Resize(rows, cols);
    canvas = std::make_unique<TextCanvas>(rows, cols);

    size_t i = row_block_spaces;
    for (auto& path : paths) {
        size_t j = col_block_spaces;
        for (State s : path) {
            positions[s] = {i, j};
            j += BLOCK_WIDTH + col_block_spaces;
        }
        i += BLOCK_HEIGHT + row_block_spaces;
    }

    for (auto& [s, pos] : positions) {
        TableId id = dfa->is_accepted(s) ? TableId::DOUBLE : TableId::ROUND;
        if (s == dfa->state_initial) id = TableId::BOLD;
        std::pair<std::string,std::string> color;
        if (dfa->is_color() && (s == dfa->state_initial || dfa->is_accepted(s))) {
            color = {iter_color(), end_color()};
        } else {
            color = {"",""};
        }
        canvas->Rect(pos, BLOCK_HEIGHT, BLOCK_WIDTH, id, color);

        Pos center = {pos.first + BLOCK_HEIGHT/2, pos.second + BLOCK_WIDTH/2};
        canvas->Text(center, std::to_string(s), Align::CENTER);
    }
}

void print(const std::string& s, const Pos &p) {
    auto [i, j] = p;
    DEBUG_OS << s << ": " << i << "," << j << "\n";
}

void DFACanvas::link(State a, State b, Token tok) { // a --> b

    auto is_neighbor = [this](int aj, int bj) {
        return aj<bj && bj-aj <= BLOCK_WIDTH + degrees*s_line_width;
    };

    size_t id = linkid[linkHash(a, b, tok)];

    auto pa = positions[a];
    auto pb = positions[b];

    size_t down_d = get_down_dist(id);
    size_t up_d = get_up_dist(id, degrees);

    auto [ai, aj] = pa;
    auto [bi, bj] = pb;

    auto func = [&]() {
        auto [ai, aj] = pa;
        auto [bi, bj] = pb;
        Pos u, v;
        if (ai < bi) {
            auto [ax, ay] = get_down_out_line(pa, down_d);
            auto [bx, by] = get_up_in_line(pb, up_d);
            canvas->Line(ax, ay);
            canvas->Line(bx, by);
            u = ay;
            v = by;
        } else if (ai > bi) {
            auto [ax, ay] = get_up_out_line(pa, up_d);
            auto [bx, by] = get_down_in_line(pb, down_d);
            canvas->Line(ax, ay);
            canvas->Line(bx, by);
            u = ay;
            v = by;
        } else {
            auto [ax, ay] = get_up_out_line(pa, up_d);
            auto [bx, by] = get_up_in_line(pb, up_d);
            canvas->Line(ax, ay);
            canvas->Line(bx, by);
            u = ay;
            v = by;
        }
        return std::make_pair(u, v);
    };

    std::string tokname;
    if (dfa->is_color()) {
        tokname = iter_color_pack(dfa->token_name(tok));
    } else {
        tokname = dfa->token_name(tok);
    }

    if (ai == bi && is_neighbor(aj, bj) && nblink.find(linkHash(a,b,0))==nblink.end()) {
        auto [ax, ay] = get_right_out_pos(pa);
        auto [bx, by] = get_left_in_pos(pb);
        canvas->Line({ax,ay}, {bx,by});
        canvas->Text({ax-1,ay}, tokname);
        canvas->Text({bx, by}, ">");
        nblink.insert(linkHash(a,b,0)); // 只连一条
        return;
    } 

    auto [u, v] = func(); // u -> v
    if (u.first == v.first) { // neighbor line
        canvas->Line(u, v);
        if (u.second < v.second) {
            canvas->Text({u.first, u.second+1}, ">");
            canvas->Text({u.first-1, u.second+1}, tokname);
        } else {
            canvas->Text({u.first, u.second-1}, "<");
            canvas->Text({u.first-1, u.second-1}, tokname, Align::RIGHT);
        }
    } else { // 
        size_t j = u.second + get_right_dist(id);
        canvas->Line(u, {u.first, j});
        canvas->Line(v, {v.first, j});
        canvas->Line({u.first, j}, {v.first, j});
        canvas->Text({u.first, u.second+1}, ">");
        canvas->Text({u.first-1, u.second+1}, tokname);
    }
}


std::vector<std::vector<State>> DFACanvas::find_paths() {

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
    return paths;
}

void DFACanvas::render() {
    std::vector<std::vector<State>> paths = find_paths();

    init(paths);

    for (State s = 0; s < dfa->dfa.size(); s++) {
        for (auto& [tok, next] : dfa->dfa[s]) {
            link(s, next, tok);
        }
    }

    postprocess();
}


void DFACanvas::dump(std::ostream& os) {
    // canvas->Dump(os);
    auto [h, w] = canvas->Size();

    size_t sz = 0;
    for (size_t i = 0; i < h; i++) {
        if (omit_row.count(i)) continue;
        for (size_t j = 0; j < w; ++j) {
            if (omit_col.count(j)) continue;
            std::string s = canvas->Get({i,j});
            os << s;
            sz += s.size();
        }
        os << "\n";
    }
    LOG_DEBUG("Canvas %lux%lu Dumped %lu Bytes\n", h, w, sz);
}

void DFACanvas::postprocess() {

    std::unordered_set<size_t> skip_rows, skip_cols;
    for (auto [s, pos] : positions) {
        auto [i, j] = pos;
        if (i + j == 0) continue;
        for (size_t row = i>1?i-1:0; row <= i + BLOCK_HEIGHT; row++) {
            skip_rows.insert(row);
        }
        for (size_t col = j>2?j-2:0; col <= j+1 + BLOCK_WIDTH; col++) {
            skip_cols.insert(col);
        }
        int ui = i;
        int uj = j+1;
        if (!str_empty(canvas->Get({ui-1,uj}))) {
            canvas->Set({ui-1,uj}, Line::ARROW_DOWN);
            canvas->Set({ui,uj}, dfa->is_accepted(s) ? 
                get_table_line(TableId::DOUBLE, TableLine::TAB_DOWN_T) : Line::DOWN_T);
        }
        uj = j + BLOCK_WIDTH - 2;
        if (!str_empty(canvas->Get({ui-1,uj}))) {
            canvas->Set({ui,uj}, dfa->is_accepted(s) ? 
                get_table_line(TableId::DOUBLE, TableLine::TAB_DOWN_T) : Line::DOWN_T);
        }

        int di = i + BLOCK_HEIGHT - 1;
        int dj = j+1;
        if (!str_empty(canvas->Get({di+1,dj}))) {
            canvas->Set({di+1,dj}, Line::ARROW_UP);
            canvas->Set({di,dj}, dfa->is_accepted(s) ? 
                get_table_line(TableId::DOUBLE, TableLine::TAB_UP_T) : Line::UP_T);
        }
        dj = j + BLOCK_WIDTH - 2;
        if (!str_empty(canvas->Get({di+1,dj}))) {
            canvas->Set({di,dj}, dfa->is_accepted(s) ? 
                get_table_line(TableId::DOUBLE, TableLine::TAB_UP_T) : Line::UP_T);
        }
    }

    auto p0 = get_left_in_pos(positions[dfa->state_initial]);
    canvas->Arrow(p0, Dir::Right);


    auto [h, w] = canvas->Size();

    for (size_t i = 0; i < h; i++) {
        if (skip_rows.count(i)) continue;
        bool vline = false;
        bool hline = false;
        for (size_t j = 0; j < w; ++j) {
            std::string s = canvas->Get({i,j});
            if (str_empty(s)) continue;
            if (s == Line::HORIZON) {
                hline = true;
            } else if (s == Line::VERTICAL) {
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

    for (size_t j = 0; j < w; ++j) {
        if (skip_cols.count(j)) continue;
        bool vline = false;
        bool hline = false;
        for (size_t i = 0; i < h; i++) {
            std::string s = canvas->Get({i,j});
            if (str_empty(s)) continue;
            if (s == Line::HORIZON) {
                hline = true;
            } else if (s == Line::VERTICAL) {
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
}