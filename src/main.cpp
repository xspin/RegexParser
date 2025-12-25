#include <fstream>
#include <exception>
#include "utils.h"
#include "Parser.h"
#include "unicode.h"
#include "GraphBox.h"
#include "RegexGenerator.h"
#include "DFA.h"
#include "DFACanvas.h"
#include "GraphSvg.h"
#include "GraphHtml.h"
#include "GraphHttp.h"


int run(int argc, char* argv[]) {
    Utils::Args args;
    int ret;

    if ((ret = parse_args(args, argc, argv))) {
        return ret;
    }

    if (args.port > 0) {
        GraphHttp http(args.port);
        http.Start();
        return 0;
    }

    args.expr = utf8_to_uhhhh(args.expr);

    if (g_debug) std::cout << "  Input Expression: " << args.expr << std::endl;

    std::unique_ptr<ExprRoot> root(regex_parse(args.expr, args.debug));
    if (!root) {
        return -1;
    }

    auto dump = [&args, &root](std::ostream& os) {
        std::string expr_str = root->stringify(args.color);

        GraphBox::set_encoding(args.utf8);
        GraphBox::set_color(args.color);
        std::unique_ptr<RootBox> box(expr_to_box(root.get()));

        // html and svg is exclusive
        if (args.format & Utils::FMT_HTML) {
            std::stringstream html_os;
            box->dump(html_os);
            GraphHtml html(expr_str, html_os.str());
            html.dump(os);
            return;
        } else if (args.format & Utils::FMT_SVG) {
            GraphSvg svg(expr_str, box->get_rows());
            svg.dump(os);
            return;
        } else if (args.format & Utils::FMT_XML) {
            os << root->xml() << std::endl;
            return;
        }

        os << "Regular Expression: " << expr_str << std::endl;

        if (args.format & Utils::FMT_TREE) {
            os << root->format(4, args.color) << std::endl;
        } 

        if (args.format & Utils::FMT_GRAPH) {
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

            DFACanvas t(&dfa);
            t.render();
            t.dump(os);
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