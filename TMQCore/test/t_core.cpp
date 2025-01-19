#include <core.h>

#include <gtest/gtest.h>

// Test case for the multiply function
TEST(MultiplyFunctionTest, PositiveNumbers) {
    EXPECT_EQ(multiply(3, 4), 12);
}

TEST(MultiplyFunctionTest, NegativeNumbers) {
    EXPECT_EQ(multiply(-3, -4), 12);
}

TEST(MultiplyFunctionTest, PositiveAndNegative) {
    EXPECT_EQ(multiply(3, -4), -12);
}

TEST(MultiplyFunctionTest, Zero) {
    EXPECT_EQ(multiply(0, 5), 0);
    EXPECT_EQ(multiply(5, 0), 0);
}

TEST(MultiplyFunctionTest, One) {
    EXPECT_EQ(multiply(1, 5), 5);
    EXPECT_EQ(multiply(5, 1), 5);
}

TEST(MultiplyFunctionTest, LargeNumbers) {
    EXPECT_EQ(multiply(1000, 2000), 2000000);
}