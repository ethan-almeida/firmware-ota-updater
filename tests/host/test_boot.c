#include "unity.h"

#include "boot.h"
#include "fakes/config_fake.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    const char *name;
    struct {
        uint32_t active;
        uint32_t pending;
        uint32_t trials;
        uint32_t min_version;
    } config;
    boot_observation_t obs;
    boot_target_t target;
    boot_action_t action;
    int trial;
    uint32_t next_active;
    uint32_t next_pending;
    uint32_t next_trials;
    uint32_t next_min;
} policy_case_t;

static const policy_case_t policy_cases[] = {
    { "no pending both valid",
      { BOOT_SLOT_A, CONFIG_SLOT_NONE, 0u, 0u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_A, BOOT_ACTION_NONE, 0, 0u, 0u, 0u, 0u },
    { "no pending active invalid",
      { BOOT_SLOT_A, CONFIG_SLOT_NONE, 0u, 0u }, { { 0, 1 }, { 0u, 2u } },
      BOOT_TARGET_RECOVERY, BOOT_ACTION_NONE, 0, 0u, 0u, 0u, 0u },
    { "no pending both invalid",
      { BOOT_SLOT_A, CONFIG_SLOT_NONE, 0u, 0u }, { { 0, 0 }, { 0u, 0u } },
      BOOT_TARGET_RECOVERY, BOOT_ACTION_NONE, 0, 0u, 0u, 0u, 0u },
    { "pending trial0 boots B",
      { BOOT_SLOT_A, BOOT_SLOT_B, 0u, 0u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_B, BOOT_ACTION_STORE_CONFIG, 1, BOOT_SLOT_A, BOOT_SLOT_B, 1u, 0u },
    { "pending trial1 bumps to 2",
      { BOOT_SLOT_A, BOOT_SLOT_B, 1u, 0u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_B, BOOT_ACTION_STORE_CONFIG, 1, BOOT_SLOT_A, BOOT_SLOT_B, 2u, 0u },
    { "pending trial2 bumps to 3",
      { BOOT_SLOT_A, BOOT_SLOT_B, 2u, 0u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_B, BOOT_ACTION_STORE_CONFIG, 1, BOOT_SLOT_A, BOOT_SLOT_B, 3u, 0u },
    { "pending trial3 exhausted rolls back",
      { BOOT_SLOT_A, BOOT_SLOT_B, 3u, 0u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_A, BOOT_ACTION_STORE_CONFIG, 0, BOOT_SLOT_A, CONFIG_SLOT_NONE, 0u, 0u },
    { "excess trials roll back",
      { BOOT_SLOT_A, BOOT_SLOT_B, 9u, 0u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_A, BOOT_ACTION_STORE_CONFIG, 0, BOOT_SLOT_A, CONFIG_SLOT_NONE, 0u, 0u },
    { "pending invalid boots A",
      { BOOT_SLOT_A, BOOT_SLOT_B, 0u, 0u }, { { 1, 0 }, { 1u, 0u } },
      BOOT_TARGET_SLOT_A, BOOT_ACTION_STORE_CONFIG, 0, BOOT_SLOT_A, CONFIG_SLOT_NONE, 0u, 0u },
    { "pending invalid active invalid recovers",
      { BOOT_SLOT_A, BOOT_SLOT_B, 0u, 0u }, { { 0, 0 }, { 0u, 0u } },
      BOOT_TARGET_RECOVERY, BOOT_ACTION_NONE, 0, 0u, 0u, 0u, 0u },
    { "excess trials pending invalid rolls back",
      { BOOT_SLOT_A, BOOT_SLOT_B, 9u, 0u }, { { 1, 0 }, { 1u, 0u } },
      BOOT_TARGET_SLOT_A, BOOT_ACTION_STORE_CONFIG, 0, BOOT_SLOT_A, CONFIG_SLOT_NONE, 0u, 0u },
    { "pending too old boots A",
      { BOOT_SLOT_A, BOOT_SLOT_B, 0u, 2u }, { { 1, 1 }, { 1u, 1u } },
      BOOT_TARGET_SLOT_A, BOOT_ACTION_STORE_CONFIG, 0, BOOT_SLOT_A, CONFIG_SLOT_NONE, 0u, 2u },
    { "pending version equal min boots B",
      { BOOT_SLOT_A, BOOT_SLOT_B, 0u, 2u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_B, BOOT_ACTION_STORE_CONFIG, 1, BOOT_SLOT_A, BOOT_SLOT_B, 1u, 2u },
    { "pending newer boots B",
      { BOOT_SLOT_A, BOOT_SLOT_B, 0u, 2u }, { { 1, 1 }, { 1u, 3u } },
      BOOT_TARGET_SLOT_B, BOOT_ACTION_STORE_CONFIG, 1, BOOT_SLOT_A, BOOT_SLOT_B, 1u, 2u },
    { "zero version pending boots",
      { BOOT_SLOT_A, BOOT_SLOT_B, 0u, 0u }, { { 1, 1 }, { 1u, 0u } },
      BOOT_TARGET_SLOT_B, BOOT_ACTION_STORE_CONFIG, 1, BOOT_SLOT_A, BOOT_SLOT_B, 1u, 0u },
    { "active B no pending",
      { BOOT_SLOT_B, CONFIG_SLOT_NONE, 0u, 2u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_B, BOOT_ACTION_NONE, 0, 0u, 0u, 0u, 0u },
    { "active B invalid recovers",
      { BOOT_SLOT_B, CONFIG_SLOT_NONE, 0u, 2u }, { { 1, 0 }, { 1u, 0u } },
      BOOT_TARGET_RECOVERY, BOOT_ACTION_NONE, 0, 0u, 0u, 0u, 0u },
    { "update back to A",
      { BOOT_SLOT_B, BOOT_SLOT_A, 0u, 2u }, { { 1, 1 }, { 2u, 2u } },
      BOOT_TARGET_SLOT_A, BOOT_ACTION_STORE_CONFIG, 1, BOOT_SLOT_B, BOOT_SLOT_A, 1u, 2u },
    { "old A rejected",
      { BOOT_SLOT_B, BOOT_SLOT_A, 0u, 5u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_B, BOOT_ACTION_STORE_CONFIG, 0, BOOT_SLOT_B, CONFIG_SLOT_NONE, 0u, 5u },
    { "pending same as active cleared",
      { BOOT_SLOT_A, BOOT_SLOT_A, 0u, 0u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_A, BOOT_ACTION_STORE_CONFIG, 0, BOOT_SLOT_A, CONFIG_SLOT_NONE, 0u, 0u },
    { "corrupt pending cleared",
      { BOOT_SLOT_A, 0x42u, 0u, 0u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_SLOT_A, BOOT_ACTION_STORE_CONFIG, 0, BOOT_SLOT_A, CONFIG_SLOT_NONE, 0u, 0u },
    { "corrupt active recovers",
      { 0x42u, CONFIG_SLOT_NONE, 0u, 0u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_RECOVERY, BOOT_ACTION_NONE, 0, 0u, 0u, 0u, 0u },
    { "corrupt active with pending recovers",
      { 0x42u, BOOT_SLOT_B, 0u, 0u }, { { 1, 1 }, { 1u, 2u } },
      BOOT_TARGET_RECOVERY, BOOT_ACTION_NONE, 0, 0u, 0u, 0u, 0u },
    { "pending valid active invalid boots pending",
      { BOOT_SLOT_A, BOOT_SLOT_B, 0u, 0u }, { { 0, 1 }, { 0u, 2u } },
      BOOT_TARGET_SLOT_B, BOOT_ACTION_STORE_CONFIG, 1, BOOT_SLOT_A, BOOT_SLOT_B, 1u, 0u },
};

static config_fake_t fake;
static config_flash_t flash;

void boot_test_set_up(void)
{
    config_fake_init(&fake);
    config_fake_bind(&fake, &flash);
}

static void test_boot_config_defaults(void)
{
    config_record_t config;
    memset(&config, 0xAA, sizeof(config));

    boot_config_defaults(&config);

    TEST_ASSERT_EQUAL_HEX32(BOOT_SLOT_A, config.active_slot);
    TEST_ASSERT_EQUAL_HEX32(CONFIG_SLOT_NONE, config.pending_slot);
    TEST_ASSERT_EQUAL_HEX32(0u, config.trial_count);
    TEST_ASSERT_EQUAL_HEX32(0u, config.min_version);
}

static void test_boot_decide_decision_table(void)
{
    const size_t count = sizeof(policy_cases) / sizeof(policy_cases[0]);

    for (size_t i = 0; i < count; i++) {
        const policy_case_t *c = &policy_cases[i];
        char msg[192];

        config_record_t config;
        boot_config_defaults(&config);
        config.active_slot = c->config.active;
        config.pending_slot = c->config.pending;
        config.trial_count = c->config.trials;
        config.min_version = c->config.min_version;

        boot_decision_t decision = boot_decide(&config, &c->obs);

        snprintf(msg, sizeof(msg), "case '%s': target", c->name);
        TEST_ASSERT_EQUAL_MESSAGE(c->target, decision.target, msg);
        snprintf(msg, sizeof(msg), "case '%s': action", c->name);
        TEST_ASSERT_EQUAL_MESSAGE(c->action, decision.action, msg);
        snprintf(msg, sizeof(msg), "case '%s': trial", c->name);
        TEST_ASSERT_EQUAL_MESSAGE(c->trial, decision.trial, msg);

        if (decision.action == BOOT_ACTION_STORE_CONFIG) {
            snprintf(msg, sizeof(msg), "case '%s': next.active", c->name);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(c->next_active, decision.next_config.active_slot, msg);
            snprintf(msg, sizeof(msg), "case '%s': next.pending", c->name);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(c->next_pending, decision.next_config.pending_slot, msg);
            snprintf(msg, sizeof(msg), "case '%s': next.trials", c->name);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(c->next_trials, decision.next_config.trial_count, msg);
            snprintf(msg, sizeof(msg), "case '%s': next.min", c->name);
            TEST_ASSERT_EQUAL_HEX32_MESSAGE(c->next_min, decision.next_config.min_version, msg);
        }
    }
}

static void test_boot_decide_null_inputs_recover(void)
{
    config_record_t config;
    boot_config_defaults(&config);
    boot_observation_t obs = { { 1, 1 }, { 1u, 2u } };

    TEST_ASSERT_EQUAL(BOOT_TARGET_RECOVERY, boot_decide(NULL, &obs).target);
    TEST_ASSERT_EQUAL(BOOT_TARGET_RECOVERY, boot_decide(&config, NULL).target);
}

static void test_confirm_on_empty_config_creates_record(void)
{
    TEST_ASSERT_EQUAL(CONFIG_OK, boot_confirm(&flash, BOOT_SLOT_B, 7u));

    config_record_t out;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_load(&flash, &out));
    TEST_ASSERT_EQUAL_HEX32(BOOT_SLOT_B, out.active_slot);
    TEST_ASSERT_EQUAL_HEX32(CONFIG_SLOT_NONE, out.pending_slot);
    TEST_ASSERT_EQUAL_HEX32(0u, out.trial_count);
    TEST_ASSERT_EQUAL_HEX32(7u, out.min_version);
}

static void test_confirm_clears_pending_and_bumps_active(void)
{
    config_record_t seed;
    memset(&seed, 0, sizeof(seed));
    seed.active_slot = BOOT_SLOT_A;
    seed.pending_slot = BOOT_SLOT_B;
    seed.trial_count = 2u;
    seed.min_version = 3u;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &seed));

    TEST_ASSERT_EQUAL(CONFIG_OK, boot_confirm(&flash, BOOT_SLOT_B, 5u));

    config_record_t out;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_load(&flash, &out));
    TEST_ASSERT_EQUAL_HEX32(BOOT_SLOT_B, out.active_slot);
    TEST_ASSERT_EQUAL_HEX32(CONFIG_SLOT_NONE, out.pending_slot);
    TEST_ASSERT_EQUAL_HEX32(0u, out.trial_count);
    TEST_ASSERT_EQUAL_HEX32(5u, out.min_version);
    TEST_ASSERT_EQUAL_HEX32(1u, out.sequence);
}

static void test_confirm_never_lowers_min_version(void)
{
    config_record_t seed;
    memset(&seed, 0, sizeof(seed));
    seed.active_slot = BOOT_SLOT_A;
    seed.pending_slot = CONFIG_SLOT_NONE;
    seed.min_version = 10u;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_store(&flash, &seed));

    TEST_ASSERT_EQUAL(CONFIG_OK, boot_confirm(&flash, BOOT_SLOT_B, 4u));

    config_record_t out;
    TEST_ASSERT_EQUAL(CONFIG_OK, config_load(&flash, &out));
    TEST_ASSERT_EQUAL_HEX32(10u, out.min_version);
    TEST_ASSERT_EQUAL_HEX32(BOOT_SLOT_B, out.active_slot);
}

static void test_confirm_rejects_invalid_slot(void)
{
    TEST_ASSERT_EQUAL(CONFIG_ERR_PARAM, boot_confirm(&flash, 2u, 5u));

    config_record_t out;
    TEST_ASSERT_EQUAL(CONFIG_ERR_EMPTY, config_load(&flash, &out));
}

static void test_confirm_null_flash_rejected(void)
{
    TEST_ASSERT_EQUAL(CONFIG_ERR_PARAM, boot_confirm(NULL, BOOT_SLOT_A, 1u));
}

void run_boot_tests(void)
{
    RUN_TEST(test_boot_config_defaults);
    RUN_TEST(test_boot_decide_decision_table);
    RUN_TEST(test_boot_decide_null_inputs_recover);
    RUN_TEST(test_confirm_on_empty_config_creates_record);
    RUN_TEST(test_confirm_clears_pending_and_bumps_active);
    RUN_TEST(test_confirm_never_lowers_min_version);
    RUN_TEST(test_confirm_rejects_invalid_slot);
    RUN_TEST(test_confirm_null_flash_rejected);
}
