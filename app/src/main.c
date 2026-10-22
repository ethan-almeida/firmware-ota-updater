#include "boot.h"
#include "config.h"
#include "config_flash_stm32f4.h"
#include "flash.h"
#include "flash_hw_stm32f4.h"
#include "uart.h"

int main(void)
{
    const boot_handshake_t *handshake = boot_handshake_get();

    uart_init();
    uart_puts("\n[app] hello from the application image\n");

    if (handshake->magic == BOOT_HANDSHAKE_MAGIC && handshake->trial_active != 0u) {
        flash_t flash;
        config_flash_t config_flash;

        flash_init(&flash, flash_hw_stm32f4(), FLASH_DEFAULT_TIMEOUT_POLLS);
        config_flash_stm32f4_init(&config_flash, &flash);

        if (flash_unlock(&flash) == FLASH_OK) {
            if (boot_confirm(&config_flash, handshake->slot, handshake->firmware_version) == CONFIG_OK) {
                uart_puts("[app] update confirmed\n");
            }
            (void)flash_lock(&flash);
        }
    }

    for (;;) {

    }
}
