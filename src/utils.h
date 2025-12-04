#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <vector>

#define APP_VERSION "0.1.0"


#define DEBUG_OS \
    std::cerr << "[DEBUG][" << __FILE_NAME__ << ":" << __LINE__ << "][" << __FUNCTION__ << "] "


#define is_odd(v) ((v)&1)
#define is_even(v) (!is_odd(v))


namespace Utils
{
enum class FMT {
    GRAPH,
    SVG,
    TREE,
    SIMPLE
};

struct Args {
    std::string output;
    std::string expr;
    FMT format;
    bool color;
    bool debug;
    bool utf8;
    int rand;
};

int parse_args(Args& args, int argc, char* argv[]);

enum class Align {
    CENTER,
    LEFT,
    RIGHT,
};

std::vector<std::pair<size_t,size_t>> split(const std::string& s, char c, size_t n=0);
std::string str_repeat(const std::string& s, size_t n);
std::string str_pad(const std::string& s, size_t n, Align align=Align::CENTER, const std::string& p=" ");


} // namespace Utils
#endif