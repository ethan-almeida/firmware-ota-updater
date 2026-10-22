#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>
#include <stdint.h>

#define CRYPTO_PUBKEY_LEN    64u
#define CRYPTO_SIGNATURE_LEN 64u
#define CRYPTO_DIGEST_LEN    32u

typedef enum {
    CRYPTO_OK = 0,
    CRYPTO_ERR_PARAM,
    CRYPTO_ERR_IMAGE,
    CRYPTO_ERR_DIGEST,
    CRYPTO_ERR_SIGNATURE,
} crypto_status_t;

crypto_status_t crypto_verify_signature(const uint8_t public_key[CRYPTO_PUBKEY_LEN],
                                        const uint8_t digest[CRYPTO_DIGEST_LEN],
                                        const uint8_t signature[CRYPTO_SIGNATURE_LEN]);

crypto_status_t crypto_verify_image(const uint8_t public_key[CRYPTO_PUBKEY_LEN],
                                    const uint8_t *raw_image, size_t total_len);

#endif
