#include <gtest/gtest.h>
#include <iostream>
#include "RegexGenerator.h"

TEST(RegexGenerator, generate) {
    RegexGenerator g;
    std::string s = g.generate(20);
    std::cout << s << std::endl;

    EXPECT_TRUE(s.size() >= 10);

    s = g.generate(50);
    std::cout << s << std::endl;

    g.seed(123);
    std::string a = g.generate(20);
    g.seed(123);
    std::string b = g.generate(20);
    EXPECT_STREQ(a.c_str(), b.c_str());
}