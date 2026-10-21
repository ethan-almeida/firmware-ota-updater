#include "unity.h"

#include "crc32.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_empty_input_finalizes_to_zero(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x00000000u, crc32_calc(NULL, 0));
}

static void test_known_answer_123456789(void)
{
    static const uint8_t msg[] = "123456789";

    TEST_ASSERT_EQUAL_HEX32(0xCBF43926u, crc32_calc(msg, sizeof(msg) - 1u));
}

static void test_known_answer_quick_brown_fox(void)
{
    static const uint8_t msg[] = "The quick brown fox jumps over the lazy dog";

    TEST_ASSERT_EQUAL_HEX32(0x414FA339u, crc32_calc(msg, sizeof(msg) - 1u));
}

static void test_single_bit_flip_changes_digest(void)
{
    uint8_t msg[] = "123456789";

    uint32_t baseline = crc32_calc(msg, sizeof(msg) - 1u);

    msg[4] ^= 0x04u;

    TEST_ASSERT_NOT_EQUAL(baseline, crc32_calc(msg, sizeof(msg) - 1u));
}

static void test_incremental_update_matches_one_shot(void)
{
    static const uint8_t msg[] = "The quick brown fox jumps over the lazy dog";
    const size_t len = sizeof(msg) - 1u;

    uint32_t crc = CRC32_SEED;
    crc = crc32_update(crc, msg, 13);
    crc = crc32_update(crc, msg + 13, len - 13);

    TEST_ASSERT_EQUAL_HEX32(crc32_calc(msg, len), crc32_finalize(crc));
}

void run_crc32_tests(void)
{
    RUN_TEST(test_empty_input_finalizes_to_zero);
    RUN_TEST(test_known_answer_123456789);
    RUN_TEST(test_known_answer_quick_brown_fox);
    RUN_TEST(test_single_bit_flip_changes_digest);
    RUN_TEST(test_incremental_update_matches_one_shot);
}
