#include <gtest/gtest.h>
#include <iostream>

#include "parser.h"

std::unique_ptr<char> fn() {
    return std::make_unique<char>(10);
}

TEST(PARSER, regex_parse) {
    std::string expr;
    std::unique_ptr<ExprNode> root;

    // test quantifier
    expr = "..??.+.*.{3,}a?b*c+d{2}|a??b*?c+?d{2}?e{1,4}";
    root = (regex_parse(expr, true));
    EXPECT_FALSE(root.get() == nullptr);
    EXPECT_STREQ(root->stringify().c_str(), expr.c_str());

    // test class 
    expr = "a[-0-9xxa-z-]b[^-A-Z_0-9]?|[\\[\\](){}\\n\\d]{4}";
    root = (regex_parse(expr, true));
    EXPECT_FALSE(root.get() == nullptr);
    EXPECT_STREQ(root->stringify().c_str(), expr.c_str());

    // test escaped
    expr = "\\\\.\\+\\?\\*\\||\\[\\]\\(\\)\\{\\}|\\b\\B\\s\\S\\w\\W\\d|[\\b\\B\\s\\S\\w\\W\\d]";
    root = (regex_parse(expr, true));
    EXPECT_FALSE(root.get() == nullptr);
    EXPECT_STREQ(root->stringify().c_str(), expr.c_str());

    // test group
    expr = "((((a)))(b){4,})|((c)(d))*(((((e)[0-9])+)))(?:no cap\\d+)";
    root = (regex_parse(expr, true));
    EXPECT_FALSE(root.get() == nullptr);
    EXPECT_STREQ(root->stringify().c_str(), expr.c_str());

    // test look
    expr = "sss(?=ahead+(abc[0-9]\\d))xxx(?!ahead+(abc[0-9]\\d))sss(?<=behind+(abc[0-9]\\d))xxx(?<!behind+(abc[0-9]\\d))";
    root = (regex_parse(expr, true));
    EXPECT_FALSE(root.get() == nullptr);
    EXPECT_STREQ(root->stringify().c_str(), expr.c_str());

    // test xhh and uhhhh
    expr = "[\xaa-\xFa\u1234-\uaBf0](\xaa-\xFa*\u1234?-\uaBf0+)";
    root = (regex_parse(expr, true));
    EXPECT_FALSE(root.get() == nullptr);
    EXPECT_STREQ(root->stringify().c_str(), expr.c_str());

    // test or
    expr = "|aa|||bb||";
    root = (regex_parse(expr, true));
    EXPECT_FALSE(root.get() == nullptr);
    EXPECT_STREQ(root->stringify().c_str(), expr.c_str());

}