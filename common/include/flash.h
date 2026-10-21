#ifndef FLASH_H
#define FLASH_H

#include <stddef.h>
#include <stdint.h>

#define FLASH_KEY1 0x45670123u
#define FLASH_KEY2 0xCDEF89ABu

#define FLASH_REG_KEYR 0x04u
#define FLASH_REG_SR   0x0Cu
#define FLASH_REG_CR   0x10u

#define FLASH_SR_EOP    (1u << 0)
#define FLASH_SR_OPERR  (1u << 1)
#define FLASH_SR_WRPERR (1u << 4)
#define FLASH_SR_PGAERR (1u << 5)
#define FLASH_SR_PGPERR (1u << 6)
#define FLASH_SR_PGSERR (1u << 7)
#define FLASH_SR_BSY    (1u << 16)

#define FLASH_SR_ERRORS (FLASH_SR_OPERR | FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR)

#define FLASH_CR_PG      (1u << 0)
#define FLASH_CR_SER     (1u << 1)
#define FLASH_CR_SNB(n)  ((uint32_t)(n) << 3)
#define FLASH_CR_PSIZE32 (2u << 8)
#define FLASH_CR_STRT    (1u << 16)
#define FLASH_CR_LOCK    (1u << 31)

#define FLASH_CR_SNB_MASK   (0xFu << 3)
#define FLASH_SECTOR_COUNT  12u

#define FLASH_DEFAULT_TIMEOUT_POLLS 50000000u

typedef struct {
    void *ctx;
    uint32_t (*read_reg)(void *ctx, uint32_t offset);
    void (*write_reg)(void *ctx, uint32_t offset, uint32_t value);
    void (*write_mem)(void *ctx, uint32_t address, uint32_t value);
    uint32_t (*read_mem)(void *ctx, uint32_t address);
} flash_hw_t;

typedef enum {
    FLASH_OK = 0,
    FLASH_ERR_PARAM,
    FLASH_ERR_LOCKED,
    FLASH_ERR_TIMEOUT,
    FLASH_ERR_ERASE,
    FLASH_ERR_PROGRAM,
    FLASH_ERR_VERIFY,
} flash_status_t;

typedef struct {
    const flash_hw_t *hw;
    uint32_t timeout_polls;
} flash_t;

void flash_init(flash_t *flash, const flash_hw_t *hw, uint32_t timeout_polls);
flash_status_t flash_unlock(flash_t *flash);
flash_status_t flash_lock(flash_t *flash);
flash_status_t flash_erase_sector(flash_t *flash, uint32_t sector);
flash_status_t flash_program_word(flash_t *flash, uint32_t address, uint32_t value);
flash_status_t flash_program(flash_t *flash, uint32_t address, const uint8_t *data, size_t len);
flash_status_t flash_read_word(flash_t *flash, uint32_t address, uint32_t *out);

#endif
