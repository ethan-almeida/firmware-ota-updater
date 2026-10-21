#include "flash_hw_stm32f4.h"

#include "stm32f407.h"

static uint32_t hw_read_reg(void *ctx, uint32_t offset)
{
    (void)ctx;
    return *(volatile uint32_t *)(uintptr_t)(FLASH_PERIPH_BASE + offset);
}

static void hw_write_reg(void *ctx, uint32_t offset, uint32_t value)
{
    (void)ctx;
    *(volatile uint32_t *)(uintptr_t)(FLASH_PERIPH_BASE + offset) = value;
}

static void hw_write_mem(void *ctx, uint32_t address, uint32_t value)
{
    (void)ctx;
    *(volatile uint32_t *)(uintptr_t)address = value;
}

static uint32_t hw_read_mem(void *ctx, uint32_t address)
{
    (void)ctx;
    return *(volatile uint32_t *)(uintptr_t)address;
}

static const flash_hw_t hw = {
    .ctx = NULL,
    .read_reg = hw_read_reg,
    .write_reg = hw_write_reg,
    .write_mem = hw_write_mem,
    .read_mem = hw_read_mem,
};

const flash_hw_t *flash_hw_stm32f4(void)
{
    return &hw;
}
