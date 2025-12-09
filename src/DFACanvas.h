#ifndef __DFACANVAS_H__
#define __DFACANVAS_H__

#include "DFA.h"
#include "TextCanvas.h"

class DFACanvas {
public:
    DFACanvas(DFA* dfa);
    void render();
    void dump(std::ostream& os=std::cout);

private:
    void init(const std::vector<std::vector<State>> &paths);
    void link(State a, State b, Token tok);

    std::vector<std::vector<State>> find_paths();
    void postprocess();

    DFA* dfa;
    std::unordered_map<State,Pos> positions;
    std::unordered_map<size_t,size_t> linkid;
    size_t degrees;
    std::unordered_set<size_t> nblink;
    std::unordered_set<size_t> omit_row;
    std::unordered_set<size_t> omit_col;

    std::unique_ptr<TextCanvas> canvas;
};

#endif // __DFACANVAS_H__