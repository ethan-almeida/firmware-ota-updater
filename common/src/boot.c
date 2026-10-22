#include "boot.h"

#include <string.h>

void boot_config_defaults(config_record_t *out)
{
    memset(out, 0, sizeof(*out));
    out->pending_slot = CONFIG_SLOT_NONE;
}

static int valid_slot(const boot_observation_t *obs, uint32_t slot)
{
    return slot <= BOOT_SLOT_B && obs->valid[slot];
}

static boot_decision_t make_decision(boot_target_t target, boot_action_t action)
{
    boot_decision_t decision;

    memset(&decision, 0, sizeof(decision));
    decision.target = target;
    decision.action = action;

    return decision;
}

static boot_decision_t decide_active(const config_record_t *config, const boot_observation_t *obs,
                                     int clear_pending)
{
    boot_decision_t decision = make_decision(BOOT_TARGET_RECOVERY, BOOT_ACTION_NONE);

    if (!valid_slot(obs, config->active_slot)) {
        return decision;
    }

    decision.target = config->active_slot == BOOT_SLOT_B ? BOOT_TARGET_SLOT_B : BOOT_TARGET_SLOT_A;

    if (clear_pending) {
        decision.action = BOOT_ACTION_STORE_CONFIG;
        decision.next_config = *config;
        decision.next_config.pending_slot = CONFIG_SLOT_NONE;
        decision.next_config.trial_count = 0u;
    }

    return decision;
}

boot_decision_t boot_decide(const config_record_t *config, const boot_observation_t *obs)
{
    if (config == NULL || obs == NULL) {
        return make_decision(BOOT_TARGET_RECOVERY, BOOT_ACTION_NONE);
    }

    if (config->active_slot > BOOT_SLOT_B) {
        return make_decision(BOOT_TARGET_RECOVERY, BOOT_ACTION_NONE);
    }

    uint32_t pending = config->pending_slot;

    if (pending == CONFIG_SLOT_NONE) {
        return decide_active(config, obs, 0);
    }

    if (pending <= BOOT_SLOT_B && pending != config->active_slot &&
        valid_slot(obs, pending) && obs->version[pending] >= config->min_version &&
        config->trial_count < BOOT_MAX_TRIALS) {
        boot_decision_t decision = make_decision(
            pending == BOOT_SLOT_B ? BOOT_TARGET_SLOT_B : BOOT_TARGET_SLOT_A,
            BOOT_ACTION_STORE_CONFIG);
        decision.trial = 1;
        decision.next_config = *config;
        decision.next_config.trial_count = config->trial_count + 1u;
        return decision;
    }

    return decide_active(config, obs, 1);
}

config_status_t boot_confirm(const config_flash_t *flash, uint32_t slot, uint32_t version)
{
    if (flash == NULL || slot > BOOT_SLOT_B) {
        return CONFIG_ERR_PARAM;
    }

    config_record_t current;

    if (config_load(flash, &current) != CONFIG_OK) {
        boot_config_defaults(&current);
    }

    current.active_slot = slot;
    current.pending_slot = CONFIG_SLOT_NONE;
    current.trial_count = 0u;

    if (version > current.min_version) {
        current.min_version = version;
    }

    return config_store(flash, &current);
}

boot_handshake_t *boot_handshake_get(void)
{
    return (boot_handshake_t *)(uintptr_t)BOOT_HANDSHAKE_ADDR;
}
