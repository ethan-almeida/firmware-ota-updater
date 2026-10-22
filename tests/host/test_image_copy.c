#include "unity.h"

#include "crypto.h"
#include "image.h"
#include "image_copy.h"

#include <stdio.h>
#include <string.h>

#define SIGNED_IMAGE_FILE "build/gen/signed_test_image.bin"
#define PUBKEY_FILE "tests/vectors/test_public_key.bin"

#define COPY_BASE 0x10000000u
#define COPY_SECTOR_SIZE 64u
#define COPY_REGION_SIZE 2048u
#define COPY_DST_OFFSET 1024u

typedef struct {
    uint8_t region[COPY_REGION_SIZE];
    int writes_left;
    int erase_count;
    int program_count;
} copy_fake_t;

static copy_fake_t fake;
static flash_ops_t ops;
static uint8_t signed_image[800];
static size_t signed_image_len;
static uint8_t pubkey[CRYPTO_PUBKEY_LEN];

static uint32_t fake_read32(void *ctx, uint32_t address)
{
    copy_fake_t *f = ctx;
    uint32_t value = 0xFFFFFFFFu;

    if (address >= COPY_BASE && address + 4u <= COPY_BASE + COPY_REGION_SIZE) {
        memcpy(&value, &f->region[address - COPY_BASE], sizeof(value));
    }
    return value;
}

static flash_ops_result_t consume_write(copy_fake_t *f)
{
    if (f->writes_left == 0) {
        return FLASH_OPS_HALTED;
    }
    if (f->writes_left > 0) {
        f->writes_left--;
    }
    return FLASH_OPS_OK;
}

static flash_ops_result_t fake_program32(void *ctx, uint32_t address, uint32_t word)
{
    copy_fake_t *f = ctx;
    uint32_t old;

    if (address < COPY_BASE || address + 4u > COPY_BASE + COPY_REGION_SIZE) {
        return FLASH_OPS_HALTED;
    }
    if (consume_write(f) != FLASH_OPS_OK) {
        return FLASH_OPS_HALTED;
    }

    memcpy(&old, &f->region[address - COPY_BASE], sizeof(old));
    old &= word;
    memcpy(&f->region[address - COPY_BASE], &old, sizeof(old));
    f->program_count++;
    return FLASH_OPS_OK;
}

static flash_ops_result_t fake_erase_sector(void *ctx, uint32_t address)
{
    copy_fake_t *f = ctx;
    uint32_t offset;

    if (address < COPY_BASE || address >= COPY_BASE + COPY_REGION_SIZE) {
        return FLASH_OPS_HALTED;
    }
    if (consume_write(f) != FLASH_OPS_OK) {
        return FLASH_OPS_HALTED;
    }

    offset = (address - COPY_BASE) & ~(COPY_SECTOR_SIZE - 1u);
    memset(&f->region[offset], 0xFF, COPY_SECTOR_SIZE);
    f->erase_count++;
    return FLASH_OPS_OK;
}

static int fake_sector_span(void *ctx, uint32_t address, uint32_t *base, uint32_t *size)
{
    uint32_t offset;

    (void)ctx;

    if (address < COPY_BASE || address >= COPY_BASE + COPY_REGION_SIZE) {
        return 0;
    }

    offset = (address - COPY_BASE) & ~(COPY_SECTOR_SIZE - 1u);
    *base = COPY_BASE + offset;
    *size = COPY_SECTOR_SIZE;

    return 1;
}

static size_t load_file(const char *path, uint8_t *out, size_t max)
{
    FILE *fp = fopen(path, "rb");
    TEST_ASSERT_NOT_NULL(fp);

    size_t n = fread(out, 1, max, fp);
    fclose(fp);

    return n;
}

void image_copy_test_set_up(void)
{
    memset(&fake, 0xFF, sizeof(fake));
    fake.writes_left = -1;
    fake.erase_count = 0;
    fake.program_count = 0;

    ops.ctx = &fake;
    ops.read32 = fake_read32;
    ops.program32 = fake_program32;
    ops.erase_sector = fake_erase_sector;
    ops.sector_span = fake_sector_span;

    signed_image_len = load_file(SIGNED_IMAGE_FILE, signed_image, sizeof(signed_image));
    TEST_ASSERT_TRUE(signed_image_len > sizeof(image_header_t));

    TEST_ASSERT_EQUAL(sizeof(pubkey), load_file(PUBKEY_FILE, pubkey, sizeof(pubkey)));
}

static void test_copy_transfers_bytes_and_erases_spanning_sectors(void)
{
    uint8_t payload[100];

    for (uint32_t i = 0; i < sizeof(payload); i++) {
        payload[i] = (uint8_t)(i * 3u + 1u);
    }
    memcpy(&fake.region[0], payload, sizeof(payload));

    TEST_ASSERT_EQUAL(IMAGE_COPY_OK,
                      image_copy_run(&ops, COPY_BASE + COPY_DST_OFFSET, COPY_BASE, sizeof(payload)));

    TEST_ASSERT_EQUAL_MEMORY(payload, &fake.region[COPY_DST_OFFSET], sizeof(payload));
    TEST_ASSERT_EQUAL(2, fake.erase_count);
    TEST_ASSERT_EQUAL(25, fake.program_count);
}

static void test_copy_pads_final_word_with_erased_bytes(void)
{
    uint8_t payload[6] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60 };
    uint8_t expected[8] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0xFF, 0xFF };

    memcpy(&fake.region[0], payload, sizeof(payload));

    TEST_ASSERT_EQUAL(IMAGE_COPY_OK,
                      image_copy_run(&ops, COPY_BASE + COPY_DST_OFFSET, COPY_BASE, sizeof(payload)));

    TEST_ASSERT_EQUAL_MEMORY(expected, &fake.region[COPY_DST_OFFSET], sizeof(expected));
}

static void test_copy_leaves_source_region_intact(void)
{
    uint8_t payload[100];
    uint8_t original[100];

    for (uint32_t i = 0; i < sizeof(payload); i++) {
        payload[i] = (uint8_t)(i * 7u + 3u);
    }
    memcpy(&fake.region[0], payload, sizeof(payload));
    memcpy(original, payload, sizeof(payload));

    TEST_ASSERT_EQUAL(IMAGE_COPY_OK,
                      image_copy_run(&ops, COPY_BASE + COPY_DST_OFFSET, COPY_BASE, sizeof(payload)));

    TEST_ASSERT_EQUAL_MEMORY(original, &fake.region[0], sizeof(original));
}

static void test_invalid_parameters_rejected(void)
{
    TEST_ASSERT_EQUAL(IMAGE_COPY_ERR_PARAM, image_copy_run(NULL, COPY_BASE, COPY_BASE, 16u));
    TEST_ASSERT_EQUAL(IMAGE_COPY_ERR_PARAM, image_copy_run(&ops, COPY_BASE + 1u, COPY_BASE, 16u));
    TEST_ASSERT_EQUAL(IMAGE_COPY_ERR_PARAM, image_copy_run(&ops, COPY_BASE, COPY_BASE + 2u, 16u));
    TEST_ASSERT_EQUAL(IMAGE_COPY_ERR_PARAM,
                      image_copy_run(&ops, COPY_BASE + COPY_REGION_SIZE, COPY_BASE, 16u));
    TEST_ASSERT_EQUAL(IMAGE_COPY_ERR_PARAM,
                      image_copy_run(&ops, COPY_BASE, COPY_BASE, COPY_REGION_SIZE + 4u));
}

static void test_zero_length_copy_is_trivially_ok(void)
{
    TEST_ASSERT_EQUAL(IMAGE_COPY_OK,
                      image_copy_run(&ops, COPY_BASE + COPY_DST_OFFSET, COPY_BASE, 0u));
    TEST_ASSERT_EQUAL(0, fake.erase_count);
    TEST_ASSERT_EQUAL(0, fake.program_count);
}

static void test_program_failure_reports_io_error(void)
{
    uint8_t payload[100];

    memset(payload, 0x5A, sizeof(payload));
    memcpy(&fake.region[0], payload, sizeof(payload));
    fake.writes_left = 3;

    TEST_ASSERT_EQUAL(IMAGE_COPY_ERR_IO,
                      image_copy_run(&ops, COPY_BASE + COPY_DST_OFFSET, COPY_BASE, sizeof(payload)));
}

static void test_power_loss_at_every_write_destination_never_verifies(void)
{
    const uint32_t dst = COPY_BASE + COPY_DST_OFFSET;
    int erases = 0;
    int words = 0;
    char msg[64];

    {
        uint32_t addr = dst;
        while (addr < dst + signed_image_len) {
            uint32_t base;
            uint32_t size;
            TEST_ASSERT_TRUE(fake_sector_span(NULL, addr, &base, &size));
            addr = base + size;
            erases++;
        }
    }
    words = (int)((signed_image_len + 3u) / 4u);

    for (int k = 0; k < erases + words; k++) {
        image_copy_test_set_up();
        memcpy(&fake.region[0], signed_image, signed_image_len);
        fake.writes_left = k;

        snprintf(msg, sizeof(msg), "cut at %d of %d", k, erases + words);

        TEST_ASSERT_EQUAL_MESSAGE(IMAGE_COPY_ERR_IO,
                                  image_copy_run(&ops, dst, COPY_BASE, signed_image_len), msg);
        TEST_ASSERT_EQUAL_MEMORY_MESSAGE(signed_image, &fake.region[0], signed_image_len, msg);

        const uint8_t *dest = &fake.region[COPY_DST_OFFSET];
        int programs_done = k > erases ? k - erases : 0;

        for (uint32_t i = 0; i < signed_image_len; i++) {
            uint8_t expected = 0xFF;
            if ((int)(i / 4u) < programs_done) {
                expected = signed_image[i];
            }
            TEST_ASSERT_EQUAL_HEX8_MESSAGE(expected, dest[i], msg);
        }

        image_header_t header;
        size_t total_len = 0;
        if (image_header_decode(dest, COPY_REGION_SIZE - COPY_DST_OFFSET, &header) == IMAGE_OK) {
            total_len = sizeof(image_header_t) + header.image_size;
        }
        if (total_len > 0u && total_len <= COPY_REGION_SIZE - COPY_DST_OFFSET) {
            TEST_ASSERT_NOT_EQUAL_MESSAGE(CRYPTO_OK, crypto_verify_image(pubkey, dest, total_len), msg);
        }
    }

    image_copy_test_set_up();
    memcpy(&fake.region[0], signed_image, signed_image_len);

    TEST_ASSERT_EQUAL(IMAGE_COPY_OK, image_copy_run(&ops, dst, COPY_BASE, signed_image_len));
    TEST_ASSERT_EQUAL_MEMORY(signed_image, &fake.region[COPY_DST_OFFSET], signed_image_len);
    TEST_ASSERT_EQUAL(CRYPTO_OK, crypto_verify_image(pubkey, &fake.region[COPY_DST_OFFSET],
                                                     signed_image_len));
}

void run_image_copy_tests(void)
{
    RUN_TEST(test_copy_transfers_bytes_and_erases_spanning_sectors);
    RUN_TEST(test_copy_pads_final_word_with_erased_bytes);
    RUN_TEST(test_copy_leaves_source_region_intact);
    RUN_TEST(test_invalid_parameters_rejected);
    RUN_TEST(test_zero_length_copy_is_trivially_ok);
    RUN_TEST(test_program_failure_reports_io_error);
    RUN_TEST(test_power_loss_at_every_write_destination_never_verifies);
}
