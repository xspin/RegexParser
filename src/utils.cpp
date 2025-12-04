#include <iostream>
#include <sstream>
#include <functional>
#include <unistd.h>
#include <numeric>
#include <exception>
#include <stdexcept> 
#include "utils.h"
#include "unicode.h"
#include "RegexGenerator.h"

namespace Utils {

std::vector<std::pair<size_t,size_t>> split(const std::string& s, char c, size_t n) {
    std::vector<std::pair<size_t,size_t>> res;
    size_t k = 0;
    size_t i = 0;
    while (i < s.size()) {
        size_t j = i;
        while (j < s.size() && s[j] != c) j++;
        res.emplace_back(i, j-i);
        i = j + 1;
        if ((n > 0 && ++k >= n) || (i == s.size() && s[j] == c)) {
            res.emplace_back(i, s.size()-i);
            break;
        }
    }
    return res;
}

std::string str_repeat(const std::string& s, size_t n) {
    std::string res;
    while (n--) {
        res += s;
    }
    return res;
}

std::string str_pad(const std::string& s, size_t k, Align align, const std::string& p) {
    std::string res;
    switch (align)
    {
    case Align::LEFT:
        res = s + str_repeat(p, k);
        break;
    case Align::RIGHT:
        res = str_repeat(p, k) + s;
        break;
    default:
        res = str_repeat(p, k/2) + s + str_repeat(p, k/2+(k&1));
    }
    return res;
}

static std::string basename(const std::string& path) {
    size_t i = path.rfind('/');
    if (i != std::string::npos) {
        return path.substr(i+1);
    }
    return path;
}

int parse_args(Args& args, int argc, char* argv[]) {
    std::string app = basename(argv[0]);

    std::stringstream help;
    help << "Usage: " << app << " [OPTIONS] [REGEX]\n"
        << "Version " << APP_VERSION << " (Tool to parse and display regular expression)\n"
        << "    -h           show this helpful usage message\n"
        << "    -v           show version info\n"
        << "    -o           specify output file path (default stdout)\n"
        << "    -f s|g|t|p   specify output format: svg/s, graph/g, tree/t, simple/p (default graph)\n"
        << "    -c           print with color\n"
        << "    -g           generate a random regular expression with specified length limit\n"
        << "    -u           utf8 encoding (default false)\n"
        << "   [REGEX]       specify regular expression input (read from stdin if without this)\n";

    args.format = FMT::GRAPH;
    args.color = false;
    args.debug = false;
    args.utf8 = false;
    args.rand = 0;

    auto parse_format = [&args](const std::string& s) {
        FMT fmt;
        if (s == "s" || s == "svg") {
            fmt = FMT::SVG;
        } else if (s == "g" || s == "graph") {
            fmt = FMT::GRAPH;
        } else if (s == "t" || s == "tree") {
            fmt = FMT::TREE;
        } else if (s == "p" || s == "simple") {
            fmt = FMT::SIMPLE;
        } else {
            return false;
        }
        args.format = fmt;
        return true;
    };

    auto parse_opt = [&]() {
        int opt;
        while ((opt = getopt(argc, argv, "g:f:o:hvcdu")) != -1) {
            switch (opt) {
                case 'd':
                    args.debug = true;
                    break;
                case 'h':
                    std::cout << help.str() << std::endl;
                    return 1;
                case 'v':
                    std::cout << app << " version " << APP_VERSION << std::endl;
                    return 1;
                case 'u':
                    args.utf8 = true;
                    break;
                case 'c':
                    args.color = true;
                    break;
                case 'g':
                    try {
                        args.rand = std::stoi(optarg);
                        if (args.rand <= 0) {
                            throw std::runtime_error("Invalid option -g");
                        }
                    } catch(const std::exception& e) {
                        std::cerr << "Failed to parse option -g: " << e.what() << std::endl; 
                        return 1;
                    }
                    break;
                case 'f':
                    if (!parse_format(optarg)) {
                        std::cerr << "Invalid format: " << optarg << std::endl;
                        return -1;
                    }
                    break;
                case 'o':
                    args.output = optarg;
                    break;
                case '?':
                    std::cerr << "Unknown option: '" << (char)optopt << "'" << std::endl;
                default:
                    std::cerr << help.str() << std::endl;
                    return -1;
            }
        }
        return 0;
    };

    while (optind < argc) {
        if (argv[optind][0] != '-') {
            args.expr = argv[optind];
            optind++;
        }
        if (parse_opt()) return -1; 
    }

    if (args.rand > 0) {
        RegexGenerator g;
        args.expr = g.generate(args.rand);
    } else if (args.expr.empty()) {
        std::getline(std::cin, args.expr); 
        return 0;
    }

    if (args.expr.empty()) {
        std::cerr << "No expression input!" << std::endl;
        std::cerr << help.str() << std::endl;
        return -1;
    }

    return 0;
}

} // namespace Utils