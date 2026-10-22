#include "unity.h"

#include "config.h"
#include "fakes/config_fake.h"

#include <string.h>

static config_fake_t fake;
static config_flash_t flash;

void config_test_set_up(void)
{
    config_fake_init(&fake);
    config_fake_bind(&fake, &flash);
}

static config_record_t make_record(uint32_t version)
{
    config_record_t record;
    memset(&record, 0, sizeof(record));
    record.active_slot = 0u;
    record.pending_slot = CONFIG_SLOT_NONE;
    record.trial_count = 0u;
    record.min_version = version;
    return record;
}

static uint32_t read_slot_magic(uint32_t slot_index)
{
    return config_fake_slot_magic(&fake, slot_index);
}

static void flip_slot_bit(uint32_t slot_index, uint32_t byte_index)
{
    config_fake_flip_byte(&fake, slot_index, byte_index);
}

static void test_load_on_empty_region_returns_error(void)
{
    config_record_t out;

    TEST_ASSERT_EQUAL(CONFIG_ERR_EMPTY, config_load(&flash, &out));
}

static void test_null_arguments_rejected(void)
{
    config_record_t record = make_record(1u);

    TEST_ASSERT_EQUAL(CONFIG_ERR_PARAM, config_load(NULL, &record));
    TEST_ASSERT_EQUAL(CONFIG_ERR_PARAM, config_load(&flash, NULL));
    TEST_ASSERT_EQUAL(CONFIG_ERR_PARAM, config_store(NULL, &record));
    TEST_ASSERT_EQUAL(CONFIG_ERR_PARAM, config_store(&flash, NULL));
}

static void test_store_then_load_roundtrip(void)
{
    config_record_t record = make_record(42u);
    record.active_slot = 1u;
    record.trial_count = 3u;

    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &record));

    config_record_t out;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_load(&flash, &out));
    TEST_ASSERT_EQUAL_HEX32(0u, out.sequence);
    TEST_ASSERT_EQUAL_HEX32(1u, out.active_slot);
    TEST_ASSERT_EQUAL_HEX32(CONFIG_SLOT_NONE, out.pending_slot);
    TEST_ASSERT_EQUAL_HEX32(3u, out.trial_count);
    TEST_ASSERT_EQUAL_HEX32(42u, out.min_version);
    TEST_ASSERT_EQUAL_HEX32(CONFIG_MAGIC, read_slot_magic(0u));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFu, read_slot_magic(1u));
}

static void test_store_overwrites_magic_of_target_slot_only_at_the_end(void)
{
    config_record_t record = make_record(1u);

    fake.writes_left = 4;

    TEST_ASSERT_EQUAL(CONFIG_ERR_IO, config_store(&flash, &record));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFu, read_slot_magic(0u));

    config_record_t out;
    TEST_ASSERT_EQUAL(CONFIG_ERR_EMPTY, config_load(&flash, &out));
}

static void test_second_store_moves_to_other_slot_and_increments_sequence(void)
{
    config_record_t first = make_record(10u);
    config_record_t second = make_record(20u);

    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &first));
    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &second));

    TEST_ASSERT_EQUAL_HEX32(CONFIG_MAGIC, read_slot_magic(0u));
    TEST_ASSERT_EQUAL_HEX32(CONFIG_MAGIC, read_slot_magic(1u));

    config_record_t out;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_load(&flash, &out));
    TEST_ASSERT_EQUAL_HEX32(1u, out.sequence);
    TEST_ASSERT_EQUAL_HEX32(20u, out.min_version);
}

static void test_third_store_reuses_first_slot(void)
{
    config_record_t record = make_record(1u);

    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &record));
    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &record));

    uint32_t slot_b_image[CONFIG_FAKE_SLOT_SIZE / 4u];
    memcpy(slot_b_image, fake.slot[1], sizeof(slot_b_image));

    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &record));

    TEST_ASSERT_EQUAL_HEX32(CONFIG_MAGIC, read_slot_magic(0u));
    TEST_ASSERT_EQUAL_MEMORY(slot_b_image, fake.slot[1], sizeof(slot_b_image));

    config_record_t out;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_load(&flash, &out));
    TEST_ASSERT_EQUAL_HEX32(2u, out.sequence);
}

static void test_power_loss_at_every_write_keeps_previous_record(void)
{
    config_record_t first = make_record(10u);
    config_record_t second = make_record(20u);

    for (int writes = 0; writes <= 7; writes++) {
        config_test_set_up();
        TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &first));
        fake.writes_left = writes;

        TEST_ASSERT_EQUAL(CONFIG_ERR_IO, config_store(&flash, &second));

        config_record_t out;
        TEST_ASSERT_EQUAL(CONFIG_OK, config_load(&flash, &out));
        TEST_ASSERT_EQUAL_HEX32(10u, out.min_version);
        TEST_ASSERT_EQUAL_HEX32(0u, out.sequence);
    }
}

static void test_full_commit_survives_and_wins(void)
{
    config_record_t first = make_record(10u);
    config_record_t second = make_record(20u);
    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &first));

    fake.writes_left = 8;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &second));

    config_record_t out;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_load(&flash, &out));
    TEST_ASSERT_EQUAL_HEX32(20u, out.min_version);
    TEST_ASSERT_EQUAL_HEX32(1u, out.sequence);
}

static void test_corrupted_newest_falls_back_to_older(void)
{
    config_record_t first = make_record(10u);
    config_record_t second = make_record(20u);
    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &first));
    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &second));

    flip_slot_bit(1u, 8);

    config_record_t out;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_load(&flash, &out));
    TEST_ASSERT_EQUAL_HEX32(10u, out.min_version);
    TEST_ASSERT_EQUAL_HEX32(0u, out.sequence);
}

void run_config_tests(void)
{
    RUN_TEST(test_load_on_empty_region_returns_error);
    RUN_TEST(test_null_arguments_rejected);
    RUN_TEST(test_store_then_load_roundtrip);
    RUN_TEST(test_store_overwrites_magic_of_target_slot_only_at_the_end);
    RUN_TEST(test_second_store_moves_to_other_slot_and_increments_sequence);
    RUN_TEST(test_third_store_reuses_first_slot);
    RUN_TEST(test_power_loss_at_every_write_keeps_previous_record);
    RUN_TEST(test_full_commit_survives_and_wins);
    RUN_TEST(test_corrupted_newest_falls_back_to_older);
}
