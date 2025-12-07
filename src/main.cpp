#include <fstream>
#include <exception>
#include "utils.h"
#include "Parser.h"
#include "unicode.h"
#include "GraphBox.h"
#include "RegexGenerator.h"
#include "DFA.h"


int run(int argc, char* argv[]) {
    Utils::Args args;
    int ret;

    if ((ret = parse_args(args, argc, argv))) {
        return ret;
    }

    args.expr = utf8_to_uhhhh(args.expr);

    std::cout << " Input Expr: " << args.expr << std::endl;

    std::unique_ptr<ExprNode> root(regex_parse(args.expr, args.debug));
    if (!root) {
        return -1;
    }

    auto dump = [&args, &root](std::ostream& os) {
        os << "Parsed Expr: " << root->stringify(args.color) << std::endl;

        if (args.debug) {
            assert(root->str() == args.expr);
        }

        if (args.format & Utils::FMT_TREE) {
            os << root->format(4, args.color) << std::endl;
        } 

        if (args.format & Utils::FMT_GRAPH) {
            GraphBox::set_encoding(args.utf8);
            GraphBox::set_color(args.color);
            std::unique_ptr<RootBox> box(expr_to_box(root.get()));
            box->dump(os);
        }

        if (args.format & Utils::FMT_NFA) {
            NFA nfa(args.color);
            nfa.generate(root.get(), args.utf8);
            nfa.dump(os);
        }

        if (args.format & Utils::FMT_DFA) {
            NFA nfa(args.color);
            nfa.generate(root.get(), args.utf8);

            DFA dfa(&nfa);
            dfa.generate();
            dfa.dump(os);

            DFAGraph g(&dfa);
            g.render();
            g.dump(os);
        } 

        if (args.format & Utils::FMT_SVG) {
            throw std::invalid_argument("Not implemented yet for svg!");
            // todo
        }
    };

    if (args.output.empty()) {
        dump(std::cout);
    } else {
        std::ofstream of(args.output);
        dump(of);
        std::cout << "Exported result to " << args.output << std::endl;
    }

    return ret;
}
int main(int argc, char* argv[]) {
    try {
        return run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}