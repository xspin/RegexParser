#include <random>
#include <ctime>
#include <sstream>
#include "unicode.h"

#define CHARS "abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#%&=_`~'\""
#define ESCAPED "nftvfr0bBsSdwW[]{}().*+?|^$\\- "
#define HEX "ABCDEFabcdef0123456789"

class RegexGenerator {
public:
    RegexGenerator(long long seed=time(nullptr)): rand_engine(seed) {
        chars = CHARS;
        escaped = ESCAPED;
        hex = HEX;
    }

    void seed(long long s) {
        rand_engine = std::mt19937(s);
    }

    std::string generate(int max_len) {
        len = max_len;
        ss.clear();
        ss.str("");
        if (prob(50)) {
            output("^");
        }
        gen_least_one();
        if (prob(50)) {
            output("$");
        }
        return ss.str();
    }

private:
    size_t rand(size_t min, size_t max) {
        std::uniform_int_distribution<> dist(min, max);
        return dist(rand_engine);
    }

    void gen_range() {
        if (prob(60)) {
            char a = rand(32, 126);
            char b = rand(32, 126);
            if (a > b) std::swap(a, b);
            output({a});
            output("-");
            output({b});
        } else {
            char a1 = rand_hex();
            char a2 = rand_hex();
            char b1 = rand_hex();
            char b2 = rand_hex();
            if (a1 > b1) std::swap(a1, b1);
            if (a2 > b2) std::swap(a2, b2);
            output({'\\', 'x', a1, a2});
            output({'\\', 'x', b1, b2});
        }
    }

    void gen_quantifier() {
        std::string s = "*?+{,}";
        size_t i = rand(0, s.size()-1);
        if (i < 3) {
            output({s[i]});
            return;
        } 
        output("{");
        if (i == 3) {
            output(std::to_string(rand(0, 20)));
        } else if (i == 4) {
            output(std::to_string(rand(0, 20)));
            output(",");
        } else {
            int a = rand(0, 20);
            int b = rand(0, 20);
            output(std::to_string(std::min(a,b)));
            output(",");
            output(std::to_string(std::max(a,b)));
        }
        if (prob(30)) {
            output("?");
        }
        output("}");
    }

    bool prob(int percent=50) {
        return rand(1, 100) <= percent;
    }

    char rand_hex() {
        size_t i = rand(0, hex.size()-1);
        return hex[i];
    }

    void gen_escaped() {
        if (prob(40)) {
            size_t i = rand(0, escaped.size()-1);
            output({'\\', escaped[i]});
        } else if (prob(50)) {
            output({'\\', 'x', rand_hex(), rand_hex()});
        } else {
            try {
                std::string uhhhh = {'\\', 'u', rand_hex(), rand_hex(), rand_hex(), rand_hex()};
                uhhhh_to_utf8(uhhhh);
                output(uhhhh);
            } catch (...) {
                // ignore invalid utf8
            }
        }
    }

    void gen_char() {
        size_t i = rand(0, chars.size()-1);
        output({chars[i]});
    }

    void gen_class() {
        if (len < 3) return;
        output("[", 1);
        if (prob(20)) {
            output("^");
        }
        int a = len;
        while (len > 0) {
            if (prob(60)) {
                gen_char();
            } else if (prob(60)) {
                gen_escaped();
            } else {
                gen_range();
            }
            if (prob(10)) {
                break;
            }
        }
        if (a == len) gen_char();
        output("]", -1);
    }

    void gen_least_one() {
        int a = len;
        gen_rand_expr();
        if (a == len) {
            gen_char();
        }
    }

    void gen_group() {
        if (len < 3) return;

        if (prob(10)) return gen_look_assertion();

        output("(", 1);
        if (prob(40)) {
            output("?:");
        }
        gen_least_one();
        output(")", -1);
    }

    void gen_look_assertion() {
        if (len < 5) return;
        std::vector<std::string> choice {
            "(?=", "(?!", "(?<=", "(?<!"
        };
        int k = rand(0, 3);
        output(choice[k], 1);
        gen_least_one();
        output(")", -1);
        next_no_quant = true;
    }

    void gen_or_branch() {
        if (len < 3) return;
        len--;
        gen_least_one();
        output("|", -1);
        gen_least_one();
    }

    void gen_rand_expr() {
        while (len > 0) {
            if (prob(40)) {
                gen_char();
            } else {
                int k = rand(1, 4);
                switch (k)
                {
                case 1: gen_escaped(); break;
                case 2: gen_class(); break;
                case 3: gen_group(); break;
                case 4: gen_or_branch(); break;
                default:
                    gen_char(); break;
                }
            }
            if (!next_no_quant && prob(30)) {
                gen_quantifier();
                next_no_quant = true;
            }
        }
    }

    bool output(const std::string& s, int n=0) {
        ss << s;
        len -= s.size() + n;
        next_no_quant = false;
        return len > 0;
    }

private:
    volatile int len;
    bool next_no_quant;
    std::mt19937 rand_engine;
    std::stringstream ss;
    std::string chars;
    std::string escaped;
    std::string hex;
};