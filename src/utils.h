#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <vector>
#include <cstdio>

#define APP_VERSION "0.4.1"

extern bool g_debug;

#define DEBUG_OS \
    std::cerr << "[DEBUG][" << __FILE_NAME__ << ":" << __LINE__ << "][" << __FUNCTION__ << "] "

#define LOG_DEBUG(fmt,...) \
    if (g_debug) fprintf(stderr, "DBG|%s:%d|%s| " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, __VA_ARGS__)

#define is_odd(v) ((v)&1)
#define is_even(v) (!is_odd(v))


namespace Utils
{
enum FMT {
    FMT_NULL = 0,
    FMT_GRAPH = 0x1,
    FMT_SVG = 0x2,
    FMT_TREE = 0x4,
    FMT_NFA = 0x10,
    FMT_DFA = 0x20,
    FMT_HTML = 0x40,
};

struct Args {
    std::string output;
    std::string expr;
    int format;
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


std::string concat(std::vector<std::string> vec, const std::string& s="");
std::vector<std::pair<size_t,size_t>> split(const std::string& s, char c, size_t n=0);
std::string str_repeat(const std::string& s, size_t n);
/*
    pad n times of p around s
 */
std::string str_pad(const std::string& s, size_t n, Align align=Align::CENTER, const std::string& p=" ");


} // namespace Utils

#endif // __UTILS_H__