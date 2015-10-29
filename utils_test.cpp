#include "gtest/gtest.h"

#include "utils.cpp"
TEST(utils, lower) {
	char data[]="aAbBcCdDEeFfGgHhIiJjKkLlMmNnOopPQqRrSsTtuUVvwWxXYyZz";
	tolower(data);
  	EXPECT_STREQ("aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz",data);
}

// Tests factorial of positive numbers.
TEST(utils, trim) {
  char data[] = " \t\n\r  abc \r\r\n\r \t";
  char out[8] = {0};
  bool ret;
  ret = trim(out, data);
  EXPECT_EQ(ret, true);
  EXPECT_STREQ("abc",out);
}

TEST(utils,time) {
	char data[65];
	gethttpnow(data);
}

TEST(signature, v2) {

}


TEST(signature, v4) {
	
}