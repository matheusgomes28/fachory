#include <gtest/gtest.h>
#include <utils/string.hpp>

TEST(StringUtilsTests, SplitsEmptyString) {
    auto const result = fachory::utils::string::split("", ',');
    EXPECT_TRUE(result.empty());
}

TEST(StringUtilsTests, SplitsTwoString) {
    auto const result = fachory::utils::string::split("one,two", ',');

    // TODO : There's an infinite loop here!
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "one");
    EXPECT_EQ(result[1], "two");
}

TEST(StringUtilsTests, SplitsThreeStrings) {
    auto const result = fachory::utils::string::split("one,two,three", ',');

    // TODO : There's an infinite loop here!
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "one");
    EXPECT_EQ(result[1], "two");
    EXPECT_EQ(result[2], "three");
}

TEST(StringUtilsTests, SplitsThreeWithEmptyOnRight) {
    auto const result = fachory::utils::string::split("one,two,", ',');

    // TODO : There's an infinite loop here!
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "one");
    EXPECT_EQ(result[1], "two");
    EXPECT_EQ(result[2], "");
}

TEST(StringUtilsTests, SplitsThreeWithEmptyOnLeft) {
    auto const result = fachory::utils::string::split(",one,two", ',');

    // TODO : There's an infinite loop here!
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "");
    EXPECT_EQ(result[1], "one");
    EXPECT_EQ(result[2], "two");
}

TEST(StringUtilsTests, SplitsTwoWithEmptyOnBothSides) {
    auto const result = fachory::utils::string::split(",", ',');

    // TODO : There's an infinite loop here!
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], "");
    EXPECT_EQ(result[1], "");
}
