#ifndef BOOT_H
#define BOOT_H

#include "config.h"

#include <stdint.h>

#define BOOT_SLOT_A 0u
#define BOOT_SLOT_B 1u

#define BOOT_MAX_TRIALS 3u

#define BOOT_HANDSHAKE_MAGIC 0x544F4F42u
#define BOOT_HANDSHAKE_ADDR 0x20000000u

typedef struct {
    uint32_t magic;
    uint32_t slot;
    uint32_t trial_active;
    uint32_t firmware_version;
} boot_handshake_t;

typedef enum {
    BOOT_TARGET_SLOT_A = 0,
    BOOT_TARGET_SLOT_B = 1,
    BOOT_TARGET_RECOVERY = 2,
} boot_target_t;

typedef enum {
    BOOT_ACTION_NONE = 0,
    BOOT_ACTION_STORE_CONFIG = 1,
} boot_action_t;

typedef struct {
    int valid[2];
    uint32_t version[2];
} boot_observation_t;

typedef struct {
    boot_target_t target;
    boot_action_t action;
    int trial;
    config_record_t next_config;
} boot_decision_t;

void boot_config_defaults(config_record_t *out);
boot_decision_t boot_decide(const config_record_t *config, const boot_observation_t *obs);
config_status_t boot_confirm(const config_flash_t *flash, uint32_t slot, uint32_t version);
boot_handshake_t *boot_handshake_get(void);

#endif
