#include "unity.h"

#include "flash.h"
#include "fakes/flash_model.h"

#define TEST_TIMEOUT_POLLS 100u
#define TEST_BASE FLASH_MODEL_BASE
#define TEST_ADDR (FLASH_MODEL_BASE + 0x100u)

static flash_model_t model;
static flash_t flash;

void flash_test_set_up(void)
{
    flash_model_init(&model);
    flash_init(&flash, flash_model_hw(&model), TEST_TIMEOUT_POLLS);
}

static void unlock(void)
{
    TEST_ASSERT_EQUAL(FLASH_OK, flash_unlock(&flash));
}

static void test_unlock_sends_keys_and_clears_lock(void)
{
    TEST_ASSERT_TRUE((flash_model_cr(&model) & FLASH_CR_LOCK) != 0u);

    unlock();

    TEST_ASSERT_EQUAL_UINT32(2u, model.key_writes);
    TEST_ASSERT_TRUE((flash_model_cr(&model) & FLASH_CR_LOCK) == 0u);
}

static void test_unlock_fails_when_lock_stays_set(void)
{
    model.ignore_unlock = 1;

    TEST_ASSERT_EQUAL(FLASH_ERR_LOCKED, flash_unlock(&flash));
}

static void test_lock_sets_lock_bit(void)
{
    unlock();

    flash_lock(&flash);

    TEST_ASSERT_TRUE((flash_model_cr(&model) & FLASH_CR_LOCK) != 0u);
}

static void test_erase_sector_fills_it_with_ff(void)
{
    unlock();
    TEST_ASSERT_EQUAL(FLASH_OK, flash_program_word(&flash, TEST_ADDR, 0x11223344u));

    TEST_ASSERT_EQUAL(FLASH_OK, flash_erase_sector(&flash, 0));

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFu, flash_model_read(&model, TEST_ADDR));
    TEST_ASSERT_EQUAL_UINT32(1u, model.erase_count);
}

static void test_erase_leaves_other_sectors_untouched(void)
{
    const uint32_t other_sector_addr = FLASH_MODEL_BASE + 0x4000u;

    unlock();
    TEST_ASSERT_EQUAL(FLASH_OK, flash_program_word(&flash, other_sector_addr, 0x55667788u));

    TEST_ASSERT_EQUAL(FLASH_OK, flash_erase_sector(&flash, 0));

    TEST_ASSERT_EQUAL_HEX32(0x55667788u, flash_model_read(&model, other_sector_addr));
}

static void test_erase_rejects_out_of_range_sector(void)
{
    unlock();

    TEST_ASSERT_EQUAL(FLASH_ERR_PARAM, flash_erase_sector(&flash, FLASH_SECTOR_COUNT));
}

static void test_erase_requires_unlock(void)
{
    TEST_ASSERT_EQUAL(FLASH_ERR_LOCKED, flash_erase_sector(&flash, 0));
}

static void test_erase_reports_hardware_error(void)
{
    unlock();
    model.op_error_flags = FLASH_SR_PGSERR;

    TEST_ASSERT_EQUAL(FLASH_ERR_ERASE, flash_erase_sector(&flash, 0));
}

static void test_erase_times_out_when_busy_stuck(void)
{
    unlock();
    model.stuck_bsy = 1u;

    TEST_ASSERT_EQUAL(FLASH_ERR_TIMEOUT, flash_erase_sector(&flash, 0));
}

static void test_program_word_roundtrip(void)
{
    unlock();

    TEST_ASSERT_EQUAL(FLASH_OK, flash_program_word(&flash, TEST_ADDR, 0x11223344u));

    uint32_t readback = 0;
    TEST_ASSERT_EQUAL(FLASH_OK, flash_read_word(&flash, TEST_ADDR, &readback));
    TEST_ASSERT_EQUAL_HEX32(0x11223344u, readback);
}

static void test_program_only_clears_bits(void)
{
    unlock();
    TEST_ASSERT_EQUAL(FLASH_OK, flash_program_word(&flash, TEST_ADDR, 0xFFFFFFFFu));

    TEST_ASSERT_EQUAL(FLASH_OK, flash_program_word(&flash, TEST_ADDR, 0xFFFFFF0Fu));

    TEST_ASSERT_EQUAL_HEX32(0xFFFFFF0Fu, flash_model_read(&model, TEST_ADDR));
}

static void test_program_rejects_unaligned_address(void)
{
    unlock();

    TEST_ASSERT_EQUAL(FLASH_ERR_PARAM, flash_program_word(&flash, TEST_ADDR + 1u, 0u));
}

static void test_program_requires_unlock(void)
{
    TEST_ASSERT_EQUAL(FLASH_ERR_LOCKED, flash_program_word(&flash, TEST_ADDR, 0u));
}

static void test_program_detects_ignored_write(void)
{
    unlock();
    model.ignored_programs = 1u;

    TEST_ASSERT_EQUAL(FLASH_ERR_VERIFY, flash_program_word(&flash, TEST_ADDR, 0x11223344u));
}

static void test_program_reports_hardware_error(void)
{
    unlock();
    model.op_error_flags = FLASH_SR_PGPERR;

    TEST_ASSERT_EQUAL(FLASH_ERR_PROGRAM, flash_program_word(&flash, TEST_ADDR, 0x11223344u));
}

static void test_program_times_out_when_busy_stuck(void)
{
    unlock();
    model.stuck_bsy = 1u;

    TEST_ASSERT_EQUAL(FLASH_ERR_TIMEOUT, flash_program_word(&flash, TEST_ADDR, 0x11223344u));
}

static void test_program_buffer_pads_final_partial_word(void)
{
    static const uint8_t data[6] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60 };

    unlock();

    TEST_ASSERT_EQUAL(FLASH_OK, flash_program(&flash, TEST_ADDR, data, sizeof(data)));

    TEST_ASSERT_EQUAL_HEX32(0x40302010u, flash_model_read(&model, TEST_ADDR));
    TEST_ASSERT_EQUAL_HEX32(0xFFFF6050u, flash_model_read(&model, TEST_ADDR + 4u));
    TEST_ASSERT_EQUAL_UINT32(2u, model.program_count);
}

static void test_program_buffer_rejects_null_data(void)
{
    unlock();

    TEST_ASSERT_EQUAL(FLASH_ERR_PARAM, flash_program(&flash, TEST_ADDR, NULL, 4u));
    TEST_ASSERT_EQUAL(FLASH_OK, flash_program(&flash, TEST_ADDR, NULL, 0u));
}

static void test_program_buffer_rejects_unaligned_address(void)
{
    static const uint8_t data[4] = { 1, 2, 3, 4 };

    unlock();

    TEST_ASSERT_EQUAL(FLASH_ERR_PARAM, flash_program(&flash, TEST_ADDR + 2u, data, sizeof(data)));
}

static void test_program_buffer_stops_on_first_failed_word(void)
{
    static const uint8_t data[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    unlock();
    model.ignored_programs = 1u;

    TEST_ASSERT_EQUAL(FLASH_ERR_VERIFY, flash_program(&flash, TEST_ADDR, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT32(1u, model.program_count);
}

static void test_read_word_rejects_unaligned_address(void)
{
    uint32_t value = 0;

    TEST_ASSERT_EQUAL(FLASH_ERR_PARAM, flash_read_word(&flash, TEST_ADDR + 1u, &value));
}

void run_flash_tests(void)
{
    RUN_TEST(test_unlock_sends_keys_and_clears_lock);
    RUN_TEST(test_unlock_fails_when_lock_stays_set);
    RUN_TEST(test_lock_sets_lock_bit);
    RUN_TEST(test_erase_sector_fills_it_with_ff);
    RUN_TEST(test_erase_leaves_other_sectors_untouched);
    RUN_TEST(test_erase_rejects_out_of_range_sector);
    RUN_TEST(test_erase_requires_unlock);
    RUN_TEST(test_erase_reports_hardware_error);
    RUN_TEST(test_erase_times_out_when_busy_stuck);
    RUN_TEST(test_program_word_roundtrip);
    RUN_TEST(test_program_only_clears_bits);
    RUN_TEST(test_program_rejects_unaligned_address);
    RUN_TEST(test_program_requires_unlock);
    RUN_TEST(test_program_detects_ignored_write);
    RUN_TEST(test_program_reports_hardware_error);
    RUN_TEST(test_program_times_out_when_busy_stuck);
    RUN_TEST(test_program_buffer_pads_final_partial_word);
    RUN_TEST(test_program_buffer_rejects_null_data);
    RUN_TEST(test_program_buffer_rejects_unaligned_address);
    RUN_TEST(test_program_buffer_stops_on_first_failed_word);
    RUN_TEST(test_read_word_rejects_unaligned_address);
}
