#ifndef __TEXTCANVAS_H__
#define __TEXTCANVAS_H__

#include <vector>
#include <string>
#include <unordered_map>
#include "utils.h"
#include "GraphBox.h"

// using VS = std::vector<std::string>;
// using VVS = std::vector<VS>;
using Pos = std::pair<size_t,size_t>;

using MS = std::unordered_map<size_t,std::string>;
using MMS = std::unordered_map<size_t,MS>;

using MU = std::unordered_map<size_t,uint8_t>;
using MMU = std::unordered_map<size_t,MU>;

class TextCanvas {
public:
    TextCanvas(size_t height=100, size_t width=100);

    void Resize(size_t height, size_t width);

    void Line(const Pos& a, const Pos& b);
    void Rect(const Pos& p, size_t height, size_t width, TableId style=TableId::DEFAULT,
        const std::pair<std::string,std::string>& color={});
    void Text(const Pos& p, const std::string& s, Align align=Align::LEFT);
    void Arrow(const Pos& p, Dir d);

    std::string Get(const Pos& p);
    void Set(const Pos& p, const std::string& s);

    void Dump(std::ostream& os=std::cout);

    // <height, width>
    std::pair<size_t,size_t> Size();

    // [spaces, empty, vline, hline, other]
    // std::vector<size_t> RowCount(size_t row);


private:
    size_t height;
    size_t width;
    MMS canvas;
    MMU overlay;
    size_t size = 0;
};

#endif // __TEXTCANVAS_H__