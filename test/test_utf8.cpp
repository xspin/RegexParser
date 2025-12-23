#include <gtest/gtest.h>
#include <iostream>

#include "unicode.h"

TEST(UTF8, utf_len) {
    size_t len = utf8_len("abc");
    EXPECT_EQ(len, 3);

    len = utf8_len("");
    EXPECT_EQ(len, 0);

    len = utf8_len("你啊");
    EXPECT_EQ(len, 2);

    len = utf8_len("a你b啊c");
    EXPECT_EQ(len, 5);
}

TEST(UTF8, utf8_to_uhhhh) {
    std::string u = utf8_to_uhhhh("中文测试");
    EXPECT_STREQ(u.c_str(), "\\u4E2D\\u6587\\u6D4B\\u8BD5");

    u = utf8_to_uhhhh("abc中文ddd");
    EXPECT_STREQ(u.c_str(), "abc\\u4E2D\\u6587ddd");

    u = utf8_to_uhhhh("abc\uabcd");
    EXPECT_STREQ(u.c_str(), "abc\\uABCD");
}

TEST(UTF8, uhhhh_to_utf8) {
    std::string s = uhhhh_to_utf8("xxx\\u4E2d\\u6587\\u6d4B\\u8BD5abcd");
    EXPECT_STREQ(s.c_str(), "xxx中文测试abcd");

    s = uhhhh_to_utf8("\\u4E00-\\u9FA5");
    std::cout << s << std::endl;
    EXPECT_STREQ(s.c_str(), "\u4E00-\u9FA5");
}

TEST(UTF8, uhhhh) {
    int v;
    v = uhhhh_cmp("\\uabcd", "\\uABCD");
    EXPECT_EQ(v, 0);

    v = uhhhh_cmp("\\uabcc", "\\uABCD");
    EXPECT_EQ(v, -1);

    v = uhhhh_cmp("\\u1bdc", "\\u1BCD");
    EXPECT_EQ(v, 1);

    EXPECT_TRUE(uhhhh_is_chinese("\\u4E00"));
    EXPECT_TRUE(uhhhh_is_chinese("\\u9FA5"));
    EXPECT_TRUE(uhhhh_is_chinese("\\u6Eaf"));

    EXPECT_FALSE(uhhhh_is_chinese("\\u4d00"));
    EXPECT_FALSE(uhhhh_is_chinese("\\u9FA6"));
}

TEST(UTF8, uhhhhh) {
    std::string s;
    s = utf8_to_uhhhh("😋");
    EXPECT_STREQ(s.c_str(), "\\U0001F60B");

    s = uhhhh_to_utf8("\\U0001F60B");
    EXPECT_STREQ(s.c_str(), "😋");

    // 代理模式
    s = uhhhh_to_utf8("\\uD83D\\uDE0B");
    EXPECT_STREQ(s.c_str(), "😋");

    // \U模式
    s = uhhhh_to_utf8("\\U0001F60B");
    EXPECT_STREQ(s.c_str(), "😋");

    // 长度不够
    s = uhhhh_to_utf8("\\U001F60B");
    EXPECT_STREQ(s.c_str(), "\\U001F60B");

    s = uhhhh_to_utf8("\\u60B");
    EXPECT_STREQ(s.c_str(), "\\u60B");

    s = uhhhh_to_utf8("\\u60B\\Uxx\\ux");
    EXPECT_STREQ(s.c_str(), "\\u60B\\Uxx\\ux");
}


TEST(UTF8, visual_width) {
    EXPECT_EQ(ansi_size("\033[0m这是测试文本\033[1;31m红色加粗\033[0m"), 15);
    EXPECT_EQ(ansi_size("\033[0;31m"), 7);
    EXPECT_EQ(ansi_size("\033[0m"), 4);

    EXPECT_EQ(utf8_len("hello测试abc"), 10);
    EXPECT_EQ(visual_width("hello测试abc"), 12);
    EXPECT_EQ(visual_width("hello\033[0m测试abc\033[0m"), 12);

    EXPECT_EQ(visual_width("┌─┐│└┘"), 6);
    
    EXPECT_EQ(ansi_size("12297829382473034410"), 0);
    EXPECT_EQ(ansi_size(""), 0);
    EXPECT_EQ(visual_width(""), 0);

    EXPECT_EQ(visual_width("😋"), 2);
}

TEST(UTF8, split) {
    std::string s = "as中文xx测试fff";
    auto r = utf8_split(s);
    EXPECT_EQ(r.size(), utf8_len(s));
    EXPECT_EQ(r[0].first, 0);
    EXPECT_EQ(r[0].second, 1);
    EXPECT_EQ(r[2].first, 2);
    EXPECT_EQ(r[2].second, 3);
    EXPECT_EQ(r[utf8_len(s)-1].first, s.size()-1);
    EXPECT_EQ(r[utf8_len(s)-1].second, 1);
}