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

std::string concat(std::vector<std::string> vec, const std::string& s) {
    std::stringstream ss;
    for (size_t i = 0; i < vec.size(); i++) {
        if (i > 0) {
            ss << s;
        }
        ss << vec[i];
    }
    return ss.str();
}

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
        << "    -f [FORMAT]  specify output format (default graph):\n"
        << "                     svg/s, graph/g, tree/t, simple/p, nfa/n, dfa/d\n"
        << "                     multiply example: g,t,d\n"
        << "    -c           print with color\n"
        << "    -g           generate a random regular expression with specified length limit\n"
        << "    -u           enable utf8 encoding (default off)\n"
        << "   [REGEX]       specify regular expression input (read from stdin if missing)\n";

    args.format = 0;
    args.color = false;
    args.debug = false;
    args.utf8 = false;
    args.rand = 0;

    auto parse_format = [&args](const std::string& arg) {
        for (auto [i, k] : split(arg, ',')) {
            FMT fmt;
            std::string s = arg.substr(i, k);
            if (s == "s" || s == "svg") {
                fmt = FMT_SVG;
            } else if (s == "g" || s == "graph") {
                fmt = FMT_GRAPH;
            } else if (s == "t" || s == "tree") {
                fmt = FMT_TREE;
            } else if (s == "p" || s == "simple") {
                fmt = FMT_SIMPLE;
            } else if (s == "n" || s == "nfa") {
                fmt = FMT_NFA;
            } else if (s == "d" || s == "dfa") {
                fmt = FMT_DFA;
            } else {
                return false;
            }
            args.format |= fmt;
        }
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

    if (args.format == 0) args.format = FMT_GRAPH;

    if (args.expr.empty()) {
        if (args.rand > 0) {
            RegexGenerator g;
            args.expr = g.generate(args.rand);
        } else {
            std::getline(std::cin, args.expr); 
        }
    }

    if (args.expr.empty()) {
        std::cerr << "No expression input!" << std::endl;
        std::cerr << help.str() << std::endl;
        return -1;
    }

    return 0;
}

} // namespace Utils