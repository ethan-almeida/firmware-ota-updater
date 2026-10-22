#include "boot.h"
#include "config.h"
#include "config_flash_stm32f4.h"
#include "crypto.h"
#include "flash.h"
#include "flash_hw_stm32f4.h"
#include "flash_map.h"
#include "image.h"
#include "image_pubkey.h"
#include "stm32f407.h"
#include "uart.h"

static flash_t flash;

static uint32_t observe_slot(uint32_t base, uint32_t max_len, uint32_t slot, boot_observation_t *obs)
{
    const uint8_t *raw = (const uint8_t *)base;
    image_header_t header;
    size_t total_len;

    if (image_header_decode(raw, max_len, &header) != IMAGE_OK) {
        return 0u;
    }

    if (header.load_address != base + IMAGE_PAYLOAD_OFFSET ||
        header.entry_address != header.load_address) {
        return 0u;
    }

    total_len = IMAGE_PAYLOAD_OFFSET + (size_t)header.image_size;
    if (total_len > max_len) {
        return 0u;
    }

    if (crypto_verify_image(image_public_key, raw, total_len) != CRYPTO_OK) {
        return 0u;
    }

    obs->valid[slot] = 1;
    obs->version[slot] = header.firmware_version;

    return header.entry_address;
}

static void __attribute__((noreturn)) jump_to_image(uint32_t entry)
{
    uint32_t sp = *(volatile const uint32_t *)entry;
    uint32_t reset = *(volatile const uint32_t *)(entry + 4u);

    SCB->VTOR = entry;

    __asm volatile(
        "cpsid i\n"
        "msr msp, %0\n"
        "bx %1\n"
        :
        : "r"(sp), "r"(reset)
        :);

    __builtin_unreachable();
}

int main(void)
{
    config_flash_t config_flash;
    config_record_t config;
    boot_observation_t obs = { { 0, 0 }, { 0u, 0u } };
    uint32_t entry[2] = { 0u, 0u };

    uart_init();
    uart_puts("\n[bootloader] ready\n");

    flash_init(&flash, flash_hw_stm32f4(), FLASH_DEFAULT_TIMEOUT_POLLS);
    config_flash_stm32f4_init(&config_flash, &flash);

    if (config_load(&config_flash, &config) != CONFIG_OK) {
        boot_config_defaults(&config);
    }

    entry[BOOT_SLOT_A] = observe_slot(FLASH_MAP_SLOT_A_BASE, FLASH_MAP_SLOT_A_SIZE, BOOT_SLOT_A, &obs);
    entry[BOOT_SLOT_B] = observe_slot(FLASH_MAP_SLOT_B_BASE, FLASH_MAP_SLOT_B_SIZE, BOOT_SLOT_B, &obs);

    boot_decision_t decision = boot_decide(&config, &obs);

    if (decision.action == BOOT_ACTION_STORE_CONFIG) {
        if (flash_unlock(&flash) == FLASH_OK) {
            (void)config_store(&config_flash, &decision.next_config);
            (void)flash_lock(&flash);
        }
    }

    if (decision.target == BOOT_TARGET_RECOVERY) {
        uart_puts("[bootloader] recovery: no bootable image\n");
        for (;;) {
        }
    }

    boot_handshake_t *handshake = boot_handshake_get();
    handshake->slot = decision.target == BOOT_TARGET_SLOT_B ? BOOT_SLOT_B : BOOT_SLOT_A;
    handshake->magic = BOOT_HANDSHAKE_MAGIC;
    handshake->trial_active = (uint32_t)decision.trial;
    handshake->firmware_version = obs.version[handshake->slot];

    if (handshake->slot == BOOT_SLOT_B) {
        uart_puts("[bootloader] boot B\n");
    } else {
        uart_puts("[bootloader] boot A\n");
    }

    uart_deinit();

    jump_to_image(entry[handshake->slot]);
}
