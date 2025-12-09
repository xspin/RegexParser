#ifndef __TEXTCANVAS_H__
#define __TEXTCANVAS_H__

#include <vector>
#include <string>
#include "utils.h"
#include "GraphBox.h"

using VS = std::vector<std::string>;
using VVS = std::vector<VS>;
using Pos = std::pair<size_t,size_t>;

class TextCanvas {
public:
    TextCanvas();

    void Resize(size_t height, size_t width);

    void Line(const Pos& a, const Pos& b);
    void Rect(const Pos& p, size_t height, size_t width, TableId style=TableId::DEFAULT,
        const std::pair<std::string,std::string>& color={});
    void Text(const Pos& p, const std::string& s, Align align=Align::LEFT);
    void Arrow(const Pos& p, Dir d);
    void Dump(std::ostream& os=std::cout);

    // <height, width>
    std::pair<size_t,size_t> Size();

    virtual void Render() final;

protected:
    VVS canvas;
    std::vector<std::vector<uint8_t>> overlay;
};

#endif // __TEXTCANVAS_H__