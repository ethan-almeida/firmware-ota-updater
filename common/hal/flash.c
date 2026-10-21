/**
    flash.c

    Flash-memory driver for STM32.
    Functions to unlock, erase, program, lock and read flash memory are included in this file


*/
#include "flash.h"
#include <string.h>

static uint32_t read_reg(flash_t *flash, uint32_t offset)
{
    return flash->hw->read_reg(flash->hw->ctx, offset);
}

static void write_reg(flash_t *flash, uint32_t offset, uint32_t value)
{
    flash->hw->write_reg(flash->hw->ctx, offset, value);
}

static flash_status_t wait_not_busy(flash_t *flash)
{
    uint32_t polls = flash->timeout_polls;

    while ((read_reg(flash, FLASH_REG_SR) & FLASH_SR_BSY) != 0u) {
        if (polls == 0u) {
            return FLASH_ERR_TIMEOUT;
        }
        polls--;
    }

    return FLASH_OK;
}

static flash_status_t check_op_result(flash_t *flash, flash_status_t error)
{
    uint32_t sr = read_reg(flash, FLASH_REG_SR);

    if ((sr & FLASH_SR_ERRORS) != 0u) {
        write_reg(flash, FLASH_REG_SR, FLASH_SR_ERRORS);
        return error;
    }

    return FLASH_OK;
}

static flash_status_t require_unlocked(flash_t *flash)
{
    if ((read_reg(flash, FLASH_REG_CR) & FLASH_CR_LOCK) != 0u) {
        return FLASH_ERR_LOCKED;
    }

    return FLASH_OK;
}

static void clear_status_flags(flash_t *flash)
{
    write_reg(flash, FLASH_REG_SR, FLASH_SR_ERRORS | FLASH_SR_EOP);
}

void flash_init(flash_t *flash, const flash_hw_t *hw, uint32_t timeout_polls)
{
    flash->hw = hw;
    flash->timeout_polls = timeout_polls;
}

flash_status_t flash_unlock(flash_t *flash)
{
    write_reg(flash, FLASH_REG_KEYR, FLASH_KEY1);
    write_reg(flash, FLASH_REG_KEYR, FLASH_KEY2);

    return require_unlocked(flash);
}

flash_status_t flash_lock(flash_t *flash)
{
    uint32_t cr = read_reg(flash, FLASH_REG_CR);

    write_reg(flash, FLASH_REG_CR, cr | FLASH_CR_LOCK);

    return FLASH_OK;
}

flash_status_t flash_erase_sector(flash_t *flash, uint32_t sector)
{
    if (sector >= FLASH_SECTOR_COUNT) {
        return FLASH_ERR_PARAM;
    }

    flash_status_t status = require_unlocked(flash);
    if (status != FLASH_OK) {
        return status;
    }

    clear_status_flags(flash);

    uint32_t cr = FLASH_CR_PSIZE32 | FLASH_CR_SER | FLASH_CR_SNB(sector);
    write_reg(flash, FLASH_REG_CR, cr);
    write_reg(flash, FLASH_REG_CR, cr | FLASH_CR_STRT);

    status = wait_not_busy(flash);
    if (status != FLASH_OK) {
        return status;
    }

    write_reg(flash, FLASH_REG_CR, FLASH_CR_PSIZE32);

    return check_op_result(flash, FLASH_ERR_ERASE);
}

flash_status_t flash_program_word(flash_t *flash, uint32_t address, uint32_t value)
{
    if ((address & 3u) != 0u) {
        return FLASH_ERR_PARAM;
    }

    flash_status_t status = require_unlocked(flash);
    if (status != FLASH_OK) {
        return status;
    }

    clear_status_flags(flash);

    write_reg(flash, FLASH_REG_CR, FLASH_CR_PSIZE32 | FLASH_CR_PG);
    flash->hw->write_mem(flash->hw->ctx, address, value);

    status = wait_not_busy(flash);
    if (status != FLASH_OK) {
        return status;
    }

    write_reg(flash, FLASH_REG_CR, FLASH_CR_PSIZE32);

    status = check_op_result(flash, FLASH_ERR_PROGRAM);
    if (status != FLASH_OK) {
        return status;
    }

    if (flash->hw->read_mem(flash->hw->ctx, address) != value) {
        return FLASH_ERR_VERIFY;
    }

    return FLASH_OK;
}

flash_status_t flash_program(flash_t *flash, uint32_t address, const uint8_t *data, size_t len)
{
    if (data == NULL && len > 0u) {
        return FLASH_ERR_PARAM;
    }
    if ((address & 3u) != 0u) {
        return FLASH_ERR_PARAM;
    }

    size_t offset = 0;
    while (offset < len) {
        size_t remaining = len - offset;
        size_t chunk = remaining > 4u ? 4u : remaining;

        uint32_t word = 0xFFFFFFFFu;
        memcpy(&word, data + offset, chunk);

        flash_status_t status = flash_program_word(flash, address + (uint32_t)offset, word);
        if (status != FLASH_OK) {
            return status;
        }

        offset += 4u;
    }

    return FLASH_OK;
}

flash_status_t flash_read_word(flash_t *flash, uint32_t address, uint32_t *out)
{
    if ((address & 3u) != 0u) {
        return FLASH_ERR_PARAM;
    }

    *out = flash->hw->read_mem(flash->hw->ctx, address);

    return FLASH_OK;
}
