#include "unity.h"

#include "crc32.h"
#include "image.h"

#include <string.h>

static image_header_t base_header;

void image_test_set_up(void)
{
    memset(&base_header, 0, sizeof(base_header));
    base_header.magic = IMAGE_MAGIC;
    base_header.header_version = IMAGE_HEADER_VERSION;
    base_header.firmware_version = 7u;
    base_header.image_size = 4096u;
    base_header.load_address = 0x08010000u;
    base_header.entry_address = 0x08010000u;
    memset(base_header.sha256, 0xAB, sizeof(base_header.sha256));
    memset(base_header.signature, 0xCD, sizeof(base_header.signature));
}

static void seal(uint8_t *raw)
{
    memcpy(raw, &base_header, sizeof(base_header));
    uint32_t crc = crc32_calc(raw, sizeof(image_header_t) - sizeof(uint32_t));
    memcpy(raw + sizeof(image_header_t) - sizeof(uint32_t), &crc, sizeof(crc));
}

static image_status_t decode_sealed(uint8_t *raw)
{
    seal(raw);
    return image_header_decode(raw, sizeof(image_header_t), NULL);
}

static void test_valid_header_decodes(void)
{
    uint8_t raw[sizeof(image_header_t)];
    seal(raw);

    image_header_t out;
    TEST_ASSERT_EQUAL(IMAGE_OK, image_header_decode(raw, sizeof(raw), &out));
    TEST_ASSERT_EQUAL_HEX32(7u, out.firmware_version);
    TEST_ASSERT_EQUAL_HEX32(4096u, out.image_size);
    TEST_ASSERT_EQUAL_HEX32(0x08010000u, out.load_address);
    TEST_ASSERT_EQUAL_HEX32(0x08010000u, out.entry_address);
    TEST_ASSERT_EQUAL_HEX8(0xAB, out.sha256[0]);
    TEST_ASSERT_EQUAL_HEX8(0xCD, out.signature[0]);
}

static void test_null_output_is_allowed(void)
{
    uint8_t raw[sizeof(image_header_t)];

    TEST_ASSERT_EQUAL(IMAGE_OK, decode_sealed(raw));
}

static void test_truncated_buffer_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    seal(raw);

    TEST_ASSERT_EQUAL(IMAGE_ERR_TRUNCATED,
                      image_header_decode(raw, sizeof(raw) - 1u, NULL));
    TEST_ASSERT_EQUAL(IMAGE_ERR_TRUNCATED, image_header_decode(NULL, sizeof(raw), NULL));
}

static void test_bad_magic_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    base_header.magic = 0xDEADBEEFu;

    TEST_ASSERT_EQUAL(IMAGE_ERR_MAGIC, decode_sealed(raw));
}

static void test_bad_header_version_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    base_header.header_version = 2u;

    TEST_ASSERT_EQUAL(IMAGE_ERR_HEADER_VERSION, decode_sealed(raw));
}

static void test_zero_size_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    base_header.image_size = 0u;

    TEST_ASSERT_EQUAL(IMAGE_ERR_SIZE, decode_sealed(raw));
}

static void test_oversized_image_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    base_header.image_size = IMAGE_PAYLOAD_MAX + 1u;

    TEST_ASSERT_EQUAL(IMAGE_ERR_SIZE, decode_sealed(raw));
}

static void test_unaligned_load_address_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    base_header.load_address = 0x08010001u;
    base_header.entry_address = 0x08010000u;

    TEST_ASSERT_EQUAL(IMAGE_ERR_ADDRESS, decode_sealed(raw));
}

static void test_unaligned_entry_address_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    base_header.entry_address = 0x08010002u;

    TEST_ASSERT_EQUAL(IMAGE_ERR_ADDRESS, decode_sealed(raw));
}

static void test_entry_before_load_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    base_header.entry_address = 0x0800FFFCu;

    TEST_ASSERT_EQUAL(IMAGE_ERR_ADDRESS, decode_sealed(raw));
}

static void test_entry_past_end_of_image_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    base_header.entry_address = base_header.load_address + base_header.image_size;

    TEST_ASSERT_EQUAL(IMAGE_ERR_ADDRESS, decode_sealed(raw));
}

static void test_entry_at_last_word_of_image_accepted(void)
{
    uint8_t raw[sizeof(image_header_t)];
    base_header.entry_address = base_header.load_address + base_header.image_size - 4u;

    TEST_ASSERT_EQUAL(IMAGE_OK, decode_sealed(raw));
}

static void test_load_address_overflow_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    base_header.load_address = 0xFFFFFFF0u;
    base_header.entry_address = 0xFFFFFFF0u;
    base_header.image_size = 4096u;

    TEST_ASSERT_EQUAL(IMAGE_ERR_ADDRESS, decode_sealed(raw));
}

static void test_corrupted_crc_rejected(void)
{
    uint8_t raw[sizeof(image_header_t)];
    seal(raw);
    raw[0] ^= 0x01u;

    TEST_ASSERT_EQUAL(IMAGE_ERR_CRC, image_header_decode(raw, sizeof(raw), NULL));
}

void run_image_tests(void)
{
    RUN_TEST(test_valid_header_decodes);
    RUN_TEST(test_null_output_is_allowed);
    RUN_TEST(test_truncated_buffer_rejected);
    RUN_TEST(test_bad_magic_rejected);
    RUN_TEST(test_bad_header_version_rejected);
    RUN_TEST(test_zero_size_rejected);
    RUN_TEST(test_oversized_image_rejected);
    RUN_TEST(test_unaligned_load_address_rejected);
    RUN_TEST(test_unaligned_entry_address_rejected);
    RUN_TEST(test_entry_before_load_rejected);
    RUN_TEST(test_entry_past_end_of_image_rejected);
    RUN_TEST(test_entry_at_last_word_of_image_accepted);
    RUN_TEST(test_load_address_overflow_rejected);
    RUN_TEST(test_corrupted_crc_rejected);
}
