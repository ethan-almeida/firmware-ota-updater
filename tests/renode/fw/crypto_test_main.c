#include "crc32.h"
#include "crypto.h"
#include "image.h"
#include "uart.h"

#include <string.h>

extern const uint8_t test_signed_image[];
extern const size_t test_signed_image_len;
extern const uint8_t test_public_key[];

static uint8_t tampered[512];
static uint32_t failures;

static void check(int condition, const char *name)
{
    uart_puts("[cryptotest] ");
    uart_puts(name);
    uart_puts(condition ? ": OK\n" : ": FAIL\n");
    if (condition == 0) {
        failures++;
    }
}

static void load_tampered(void)
{
    memcpy(tampered, test_signed_image, test_signed_image_len);
}

static void reseal_header(void)
{
    uint32_t crc = crc32_calc(tampered, sizeof(image_header_t) - sizeof(uint32_t));
    memcpy(tampered + sizeof(image_header_t) - sizeof(uint32_t), &crc, sizeof(crc));
}

static void run_checks(void)
{
    check(crypto_verify_image(test_public_key, test_signed_image, test_signed_image_len) == CRYPTO_OK,
          "valid");

    load_tampered();
    tampered[sizeof(image_header_t) + 10] ^= 0x01u;
    check(crypto_verify_image(test_public_key, tampered, test_signed_image_len) == CRYPTO_ERR_DIGEST,
          "tampered-payload");

    load_tampered();
    image_header_t header;
    memcpy(&header, tampered, sizeof(header));
    header.signature[0] ^= 0x01u;
    memcpy(tampered, &header, sizeof(header));
    reseal_header();
    check(crypto_verify_image(test_public_key, tampered, test_signed_image_len) == CRYPTO_ERR_SIGNATURE,
          "tampered-signature");

    load_tampered();
    memcpy(&header, tampered, sizeof(header));
    header.sha256[0] ^= 0x01u;
    memcpy(tampered, &header, sizeof(header));
    reseal_header();
    check(crypto_verify_image(test_public_key, tampered, test_signed_image_len) == CRYPTO_ERR_DIGEST,
          "tampered-digest");

    uint8_t wrong_key[CRYPTO_PUBKEY_LEN];
    memcpy(wrong_key, test_public_key, CRYPTO_PUBKEY_LEN);
    wrong_key[0] ^= 0x01u;
    check(crypto_verify_image(wrong_key, test_signed_image, test_signed_image_len) == CRYPTO_ERR_SIGNATURE,
          "wrong-key");

    check(crypto_verify_image(test_public_key, test_signed_image, test_signed_image_len - 1) == CRYPTO_ERR_IMAGE,
          "truncated");
}

int main(void)
{
    uart_init();

    check(test_signed_image_len <= sizeof(tampered), "image-fits");

    run_checks();

    if (failures == 0u) {
        uart_puts("[cryptotest] PASS\n");
    }

    for (;;) {
    }
}
