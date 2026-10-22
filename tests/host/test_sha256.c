#include "unity.h"

#include "hexutil.h"
#include "sha256.h"

#include <string.h>

static uint8_t digest[SHA256_DIGEST_LEN];
static uint8_t buffer[1024 * 1024 + 8];

void sha256_test_set_up(void) {}

static void assert_hash(const uint8_t *data, size_t len, const char *expected_hex)
{
    uint8_t expected[SHA256_DIGEST_LEN];
    size_t n = hex_decode(expected_hex, expected, sizeof(expected));

    TEST_ASSERT_EQUAL_UINT32(SHA256_DIGEST_LEN, n);
    sha256(data, len, digest);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, digest, SHA256_DIGEST_LEN);
}

static void fill_pattern(uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        data[i] = (uint8_t)((i * 7u + 3u) & 0xFFu);
    }
}

static void test_empty_message(void)
{
    assert_hash((const uint8_t *)"", 0,
                "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

static void test_abc(void)
{
    assert_hash((const uint8_t *)"abc", 3,
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

static void test_fifty_six_byte_two_block_message(void)
{
    static const char *msg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

    assert_hash((const uint8_t *)msg, strlen(msg),
                "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

static void test_hundred_twelve_byte_two_block_message(void)
{
    static const char *msg = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmno"
                             "ijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";

    assert_hash((const uint8_t *)msg, strlen(msg),
                "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1");
}

static void test_sample_message(void)
{
    assert_hash((const uint8_t *)"sample", 6,
                "af2bdbe1aa9b6ec1e2ade1d694f41fc71a831d0268e9891562113d8a62add1bf");
}

static void test_test_message(void)
{
    assert_hash((const uint8_t *)"test", 4,
                "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08");
}

static void test_padding_boundary_lengths(void)
{
    static const struct {
        size_t len;
        const char *expected;
    } cases[] = {
        { 54u, "160bbf14b458c877b7049e7cb5771dd653930f97d20bdd8ee795c16062906233" },
        { 55u, "e7313d333c272e639f790978283f9eb392e843d0f29b7016828bb1daa4aac70b" },
        { 56u, "4324d65f3c103567f5589c710bc08f8523f929a9272e3af36fc968e52abc6c27" },
        { 57u, "35df609437dcfea3279283ab79fd554e2bf78f8f7ae2de532d8ee300b09e8f73" },
        { 63u, "81c80242132f230c3bd41b3e63bbcff16107339549214a99614ff26664625055" },
        { 64u, "39e3d7b6b5d075d37d053ad89b24b41bef4f3c29760c84447cab3f3be1882241" },
        { 65u, "aacca6ff74fdbb296d165a45cecfa04e5127bc008770fbbdd48006f2d2fae95e" },
        { 127u, "a8d23e75d936f303d248888d9b165ee543f4cbafcad3c9dd2a79bd84faa11d07" },
        { 128u, "d2742f1f4ac6bb7ca2b239ee18402ba8b3f9f8e652d2a72973c2b9ba11c08cf6" },
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        fill_pattern(buffer, cases[i].len);
        assert_hash(buffer, cases[i].len, cases[i].expected);
    }
}

static void test_one_million_a(void)
{
    memset(buffer, 'a', 1000000u);
    assert_hash(buffer, 1000000u,
                "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

static void test_chunked_updates_match_one_shot(void)
{
    static const size_t split_points[] = { 1u, 3u, 7u, 55u, 56u, 63u, 64u, 65u, 200u };
    uint8_t one_shot[SHA256_DIGEST_LEN];
    uint8_t chunked[SHA256_DIGEST_LEN];

    fill_pattern(buffer, 300u);
    sha256(buffer, 300u, one_shot);

    for (size_t i = 0; i < sizeof(split_points) / sizeof(split_points[0]); i++) {
        sha256_ctx_t ctx;

        sha256_init(&ctx);
        sha256_update(&ctx, buffer, split_points[i]);
        sha256_update(&ctx, buffer + split_points[i], 300u - split_points[i]);
        sha256_final(&ctx, chunked);

        TEST_ASSERT_EQUAL_HEX8_ARRAY(one_shot, chunked, SHA256_DIGEST_LEN);
    }
}

static void test_context_reuse_after_final(void)
{
    uint8_t first[SHA256_DIGEST_LEN];
    uint8_t second[SHA256_DIGEST_LEN];
    sha256_ctx_t ctx;

    sha256((const uint8_t *)"abc", 3, first);

    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)"ab", 2);
    sha256_update(&ctx, (const uint8_t *)"c", 1);
    sha256_final(&ctx, second);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(first, second, SHA256_DIGEST_LEN);

    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)"abc", 3);
    sha256_final(&ctx, second);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(first, second, SHA256_DIGEST_LEN);
}

void run_sha256_tests(void)
{
    RUN_TEST(test_empty_message);
    RUN_TEST(test_abc);
    RUN_TEST(test_fifty_six_byte_two_block_message);
    RUN_TEST(test_hundred_twelve_byte_two_block_message);
    RUN_TEST(test_sample_message);
    RUN_TEST(test_test_message);
    RUN_TEST(test_padding_boundary_lengths);
    RUN_TEST(test_one_million_a);
    RUN_TEST(test_chunked_updates_match_one_shot);
    RUN_TEST(test_context_reuse_after_final);
}
