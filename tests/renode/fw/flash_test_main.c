#include "config.h"
#include "config_flash_stm32f4.h"
#include "flash.h"
#include "flash_hw_stm32f4.h"
#include "flash_map.h"
#include "uart.h"

#include <string.h>

#define TEST_SECTOR 11u
#define TEST_BASE   FLASH_MAP_CONFIG_B_BASE
#define TEST_WORDS  16u

static flash_t flash;
static uint32_t failures;

static void expect(int condition, const char *name)
{
    if (condition == 0) {
        uart_puts("[flashtest] FAIL: ");
        uart_puts(name);
        uart_puts("\n");
        failures++;
    }
}

static void run_flash_checks(void)
{
    expect(flash_unlock(&flash) == FLASH_OK, "unlock");
    expect(flash_erase_sector(&flash, TEST_SECTOR) == FLASH_OK, "erase");

    for (uint32_t i = 0; i < TEST_WORDS; i++) {
        uint32_t value = 0xA5000000u | i;
        expect(flash_program_word(&flash, TEST_BASE + (i * 4u), value) == FLASH_OK, "program");
    }

    for (uint32_t i = 0; i < TEST_WORDS; i++) {
        uint32_t value = 0;
        expect(flash_read_word(&flash, TEST_BASE + (i * 4u), &value) == FLASH_OK, "read");
        expect(value == (0xA5000000u | i), "verify");
    }

    expect(flash_program_word(&flash, TEST_BASE + 1u, 0u) == FLASH_ERR_PARAM, "unaligned");
    expect(flash_erase_sector(&flash, FLASH_SECTOR_COUNT) == FLASH_ERR_PARAM, "sector-range");
}

static void run_config_checks(void)
{
    config_flash_t config;
    config_flash_stm32f4_init(&config, &flash);

    config_record_t record;
    memset(&record, 0, sizeof(record));
    record.active_slot = 0u;
    record.pending_slot = CONFIG_SLOT_NONE;
    record.min_version = 5u;

    expect(config_store(&config, &record) == CONFIG_OK, "config-store");

    config_record_t loaded;
    memset(&loaded, 0, sizeof(loaded));
    expect(config_load(&config, &loaded) == CONFIG_OK, "config-load");
    expect(loaded.sequence == 0u, "config-sequence");
    expect(loaded.min_version == 5u, "config-version");
}

int main(void)
{
    uart_init();

    flash_init(&flash, flash_hw_stm32f4(), FLASH_DEFAULT_TIMEOUT_POLLS);

    run_flash_checks();
    run_config_checks();

    expect(flash_lock(&flash) == FLASH_OK, "lock");

    if (failures == 0u) {
        uart_puts("[flashtest] PASS\n");
    }

    for (;;) {
    }
}
