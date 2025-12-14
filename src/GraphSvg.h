#ifndef GRAPH_SVG_H
#define GRAPH_SVG_H

#include "GraphBox.h"

#define PIX_SIZE 14

#define COLOR_TEXT "lightgrey"
#define COLOR_ESCAPED "lightgreen"
#define COLOR_ANCHOR "lightpink"
#define COLOR_RANGE "lightblue"

static inline std::string get_color(BoxType type) {
    switch (type) {
        case BoxType::ESCAPED:
            return COLOR_ESCAPED;
        case BoxType::ANCHOR:
            return COLOR_ANCHOR;
        case BoxType::RANGE:
            return COLOR_RANGE;
        default:
            return COLOR_TEXT;
    }
}

template<typename T>
static inline std::string strwraperr(const T& t) {
    return "\"" + std::to_string(t) + "\"";
}

class SvgElement {
public:
    SvgElement() {
    }

    virtual ~SvgElement() {
    }

    virtual std::string str() =0;

protected:
    size_t x;
    size_t y;
};

class SvgText: public SvgElement {
    std::string content;
    std::string color;
    size_t font_size = PIX_SIZE;

public:
    SvgText(size_t x, size_t y, const std::string& content, const std::string& color="")
    : content(content), color(color) {
        this->x = x;
        this->y = y;
    }

    std::string str() override {
        std::stringstream ss;

        if (!color.empty()) {
            ss << "<rect x=" << strwraperr(x)
            << " y=" << strwraperr(y)
            << " width=" << strwraperr(content.size() * font_size * 0.7)
            << " height=" << strwraperr(font_size)
            << " fill=\"" << color << "\""
            << " rx=\"3\" ry=\"3\""
            << "/>";
        }
        ss << "<text x=" << strwraperr(x)
           << " y=" << strwraperr(y)
           << " font-size=" << strwraperr(font_size)
           << " font-family=\"Consolas, Monaco, 'Courier New', monospace\""
           << " dominant-baseline=\"hanging\""
           << " text-anchor=\"start\""
           << " white-space=\"pre\""
           << ">" << content << "</text>";
        return ss.str();
    }
};

class SvgLine: public SvgElement {
    size_t x2;
    size_t y2;

public:
    SvgLine(size_t x1, size_t y1, size_t x2, size_t y2): x2(x2), y2(y2) {
        this->x = x1;
        this->y = y1;
    }

    std::string str() override {
        std::stringstream ss;
        ss << "<line x1=" << strwraperr(x)
           << " y1=" << strwraperr(y)
           << " x2=" << strwraperr(x2)
           << " y2=" << strwraperr(y2)
           << " stroke-width=" << strwraperr(1)
           << " stroke=\"black\""
           << "/>";
        return ss.str();
    }
};

class SvgRect: public SvgElement {
    size_t width;
    size_t height;

public:
    SvgRect(size_t x, size_t y, size_t width, size_t height): width(width), height(height) {
        this->x = x;
        this->y = y;
    }
    std::string str() override {
        std::stringstream ss;
        ss << "<rect x=" << strwraperr(x)
           << " y=" << strwraperr(y)
           << " width=" << strwraperr(width)
           << " height=" << strwraperr(height)
           << " stroke-width=" << strwraperr(1)
           << " stroke=\"black\""
           << " fill=\"none\""
           << " rx=\"5\" ry=\"5\""
           << "/>";
        return ss.str();
    }
};

static inline std::string spaces_nbsp(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (c == ' ') {
            res += "&nbsp;";
        } else {
            res += c;
        }
    }
    return res;
}

class SvgRoot: public SvgElement {
    std::string expr;
    size_t width;
    size_t height;
    std::vector<std::unique_ptr<SvgElement>> elements;
    const size_t margin_y = 2;

public:
    SvgRoot(RootBox* root, const std::string& expr): expr(expr) {
        std::string sexpr = "Regex: " + expr;
        std::stringstream ss;
        this->width = std::max(sexpr.size(), (root->get_width()+2)) * PIX_SIZE * 0.7;
        this->height = (root->get_height()+2+margin_y) * PIX_SIZE;

        bool first = true;
        for (const auto& row : root->get_rows()) {
            ss << "<tspan x=\"10\"";
            if (!first) ss << " dy=" << strwraperr(PIX_SIZE);
            ss << ">" << spaces_nbsp(row) << "</tspan>\n";
            first = false;
        }

        elements.emplace_back(std::make_unique<SvgText>(0, PIX_SIZE*2, ss.str()));
        elements.emplace_back(std::make_unique<SvgText>(10, 5, sexpr));
        return;
        // todo

//  NORMAL,
//     RANGE,
//     CLASS,
//     GROUP,
//     ASSERTION,
//     BRANCH,
//     EMPTY,
//     LINK,
//     ANCHOR,
//     QUANTIFIER,
//     ESCAPED,
//     ROOT,
        travel_box(root, [this](GraphBox* box) {
            BoxType type = box->get_type();
            auto [tx, ty] = box->get_position();
            switch (type) {
                case BoxType::NORMAL:
                case BoxType::RANGE:
                case BoxType::ESCAPED:
                case BoxType::ANCHOR:
                    add_text(tx, ty, box->get_raw(), get_color(type));
                    break;
                case BoxType::ASSERTION:
                case BoxType::GROUP:
                case BoxType::CLASS:
                    add_rect(tx, ty, box->get_width(), box->get_height());
                default:
                    break;
            }
        });
    }

    void add_text(size_t tx, size_t ty, const std::string& content, const std::string& color="") {
        ty += margin_y;
        elements.emplace_back(std::make_unique<SvgText>(tx*PIX_SIZE, ty*PIX_SIZE, content, color));
    }

    void add_rect(size_t tx, size_t ty, size_t width, size_t height) {
        ty += margin_y;
        tx++;
        width -= 2;
        elements.emplace_back(std::make_unique<SvgRect>(tx*PIX_SIZE, ty*PIX_SIZE, width*PIX_SIZE, height*PIX_SIZE));
        elements.emplace_back(std::make_unique<SvgLine>(
            (tx-1)*PIX_SIZE, (ty + height/2)*PIX_SIZE,
            tx*PIX_SIZE, (ty + height/2)*PIX_SIZE
        ));
        elements.emplace_back(std::make_unique<SvgLine>(
            (tx + width)*PIX_SIZE, (ty + height/2)*PIX_SIZE,
            (tx + width + 1)*PIX_SIZE, (ty + height/2)*PIX_SIZE
        ));
    }

    void add_element(std::unique_ptr<SvgElement> elem) {
        elements.emplace_back(std::move(elem));
    }

    std::string str() override {
        std::stringstream ss;
        ss << "<svg width=" << strwraperr(width)
           << " height=" << strwraperr(height)
           << " xmlns=" << "\"http://www.w3.org/2000/svg\"" << ">\n";
        for (auto& elem: elements) {
            ss << "  " << elem->str() << "\n";
        }
        ss << "</svg>\n";
        return ss.str();
    }

    void dump(std::ostream& os) {
        os << str();
    }
};

#endif


