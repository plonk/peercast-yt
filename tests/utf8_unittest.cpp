#include <gtest/gtest.h>
#include "utf8.h"

// int utf8_encode(const char *from, char **to);
// int utf8_decode(const char *from, char **to);

TEST(UTF8Test, encodeWorks) {
    char *out;

    utf8_encode("", &out);
    ASSERT_STREQ("", out);
    free(out);

    utf8_encode("ABC", &out);
    ASSERT_STREQ("ABC", out);
    free(out);

    utf8_encode("\x82\xd0\x82\xe7\x82\xdf", &out);
    ASSERT_STREQ("ひらめ", out);
    free(out);
}

TEST(UTF8Test, decodeWorks) {
    char *out;

    utf8_decode("", &out);
    ASSERT_STREQ("", out);
    free(out);

    utf8_decode("ABC", &out);
    ASSERT_STREQ("ABC", out);
    free(out);

    utf8_decode("ひらめ", &out);
    ASSERT_STREQ("\x82\xd0\x82\xe7\x82\xdf", out);
    free(out);
}

