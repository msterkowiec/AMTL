#include <gtest/gtest.h>

#include "amt_vector.h"

TEST(AMTTest, BasicTest) {

	amt::vector<int> vec;
	EXPECT_EQ(vec.size(), 0);
	vec.emplace_back(10);
	EXPECT_EQ(vec.size(), 1);
}