import hello;
#include <gtest/gtest.h>

TEST(HelloFeature, BuildsMessageWithConstexprValidation) {
    const auto message = hello::build_message("modules");

    EXPECT_STREQ(message, "Hello from C++23 modules");
}

TEST(HelloFeature, RejectsEmptyLanguage) {
    const auto message = hello::build_message("");

    EXPECT_STREQ(message, "Invalid language");
}
