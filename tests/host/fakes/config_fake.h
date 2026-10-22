#ifndef CONFIG_FAKE_H
#define CONFIG_FAKE_H

#include "config.h"

#include <stdint.h>

#define CONFIG_FAKE_SLOT_A 0x10000000u
#define CONFIG_FAKE_SLOT_B 0x20000000u
#define CONFIG_FAKE_SLOT_SIZE 128u

typedef struct {
    uint8_t slot[2][CONFIG_FAKE_SLOT_SIZE];
    int writes_left;
} config_fake_t;

void config_fake_init(config_fake_t *fake);
void config_fake_bind(config_fake_t *fake, config_flash_t *flash);
uint32_t config_fake_slot_magic(const config_fake_t *fake, uint32_t slot);
void config_fake_flip_byte(config_fake_t *fake, uint32_t slot, uint32_t byte_index);

#endif
