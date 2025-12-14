#include <gtest/gtest.h>
#include <iostream>
#include "GraphBox.h"

void dump_rows(const Rows& rows) {
    for (const auto& row : rows) {
        std::cout << row << "\n";
    }
}

TEST(GraphBox, box_table) {
    TableId id = TableId::DEFAULT;
    Rows lines = {"abc", "defg", "hij"};
    auto [rows, span] = box_table(id, lines, "123");
    dump_rows(rows);
    EXPECT_EQ(rows.size(), 5);
    EXPECT_EQ(span.left, 2);
    EXPECT_EQ(span.top, 1);
    EXPECT_EQ(span.right, 2);
    EXPECT_EQ(span.bottom, 1);
}

TEST(GraphBox, box_bottom) {
    Rows lines = {"abcd", "defg", "hijx"};
    auto [rows, span] = box_bottom(lines, "*", false);
    dump_rows(rows);
    EXPECT_EQ(rows.size(), 5);
    EXPECT_EQ(span.left, 2);
    EXPECT_EQ(span.top, 0);
    EXPECT_EQ(span.right, 2);
    EXPECT_EQ(span.bottom, 1);
}

TEST(GraphBox, box_expand) {
    Rows lines = {"abcd", "dexs", "fghi"};
    auto [rows, span] = box_expand(lines, 7, Line::HORIZON, Align::CENTER);
    dump_rows(rows);
    EXPECT_EQ(rows.size(), 3);
    EXPECT_EQ(span.left, 1);
    EXPECT_EQ(span.right, 2);
    EXPECT_EQ(span.top, 0);
    EXPECT_EQ(span.bottom, 0);

    auto t = box_expand(lines, 7, Line::HORIZON, Align::LEFT);
    dump_rows(t.first);
    span = t.second;
    EXPECT_EQ(span.left, 0);
    EXPECT_EQ(span.right, 3);
    EXPECT_EQ(span.top, 0);
    EXPECT_EQ(span.bottom, 0);
}