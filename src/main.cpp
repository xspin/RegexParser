#include <fstream>
#include "utils.h"
#include "parser.h"
#include "unicode.h"
#include "GraphBox.h"
#include "RegexGenerator.h"
#include <exception>


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

        if (args.format == Utils::FMT::TREE) {
            os << root->format(4, args.color) << std::endl;
        } else if (args.format == Utils::FMT::GRAPH) {
            GraphBox::set_encoding(args.utf8);
            GraphBox::set_color(args.color);
            std::unique_ptr<RootBox> box(expr_to_box(root.get()));
            box->dump(os);
        } else if (args.format == Utils::FMT::SVG) {
            // todo
        } else if (args.format == Utils::FMT::SIMPLE) {
            // do nothing
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