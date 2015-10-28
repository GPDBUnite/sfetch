#include "gtest/gtest.h"

TEST(Hello, Zero) {
  EXPECT_EQ(1, 1);
}

// Tests factorial of positive numbers.
TEST(Hello, Positive) {
  EXPECT_EQ(1, 1);
  EXPECT_EQ(2, 2);
  EXPECT_EQ(6, 6);
  EXPECT_EQ(40320, 40320);
}

