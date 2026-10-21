#ifndef FLASH_MODEL_H
#define FLASH_MODEL_H

#include <stdint.h>

#include "flash.h"

#define FLASH_MODEL_BASE   0x08000000u
#define FLASH_MODEL_SIZE   (64u * 1024u)
#define FLASH_MODEL_SECTORS 4u

typedef struct {
    flash_hw_t hw;
    uint32_t regs[8];
    uint32_t words[FLASH_MODEL_SIZE / 4u];
    uint32_t bsy_polls_left;
    uint32_t stuck_bsy;
    uint32_t op_error_flags;
    uint32_t ignored_programs;
    uint32_t key_state;
    uint32_t key_writes;
    uint32_t erase_count;
    uint32_t program_count;
    int ignore_unlock;
} flash_model_t;

void flash_model_init(flash_model_t *model);
const flash_hw_t *flash_model_hw(flash_model_t *model);
uint32_t flash_model_cr(const flash_model_t *model);
uint32_t flash_model_sr(const flash_model_t *model);
uint32_t flash_model_read(const flash_model_t *model, uint32_t address);

#endif
