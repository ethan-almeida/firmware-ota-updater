/**
    crypto.c

    Verifies a firmware img is authentic and has not been modified.

*/

#include "crypto.h"
#include "image.h"
#include "sha256.h"
#include "uECC.h"
#include <string.h>

// check for public key, digest and signature
crypto_status_t crypto_verify_signature(const uint8_t public_key[CRYPTO_PUBKEY_LEN],
                                        const uint8_t digest[CRYPTO_DIGEST_LEN],
                                        const uint8_t signature[CRYPTO_SIGNATURE_LEN])
{
    if (public_key == NULL || digest == NULL || signature == NULL) {
        return CRYPTO_ERR_PARAM;
    }

    if (uECC_verify(public_key, digest, CRYPTO_DIGEST_LEN, signature, uECC_secp256r1()) != 1) {
        return CRYPTO_ERR_SIGNATURE;
    }

    return CRYPTO_OK;
}

// check and validate public key, image data and image header
crypto_status_t crypto_verify_image(const uint8_t public_key[CRYPTO_PUBKEY_LEN], const uint8_t *raw_image, size_t total_len)
{
    if (public_key == NULL || raw_image == NULL) {
        return CRYPTO_ERR_PARAM;
    }

    image_header_t header;
    if (image_header_decode(raw_image, total_len, &header) != IMAGE_OK) {
        return CRYPTO_ERR_IMAGE;
    }

    if (total_len != sizeof(image_header_t) + (size_t)header.image_size) {
        return CRYPTO_ERR_IMAGE;
    }

    uint8_t digest[CRYPTO_DIGEST_LEN];
    sha256(raw_image + sizeof(image_header_t), header.image_size, digest);

    if (memcmp(digest, header.sha256, sizeof(digest)) != 0) {
        return CRYPTO_ERR_DIGEST;
    }

    if (crypto_verify_signature(public_key, digest, header.signature) != CRYPTO_OK) {
        return CRYPTO_ERR_SIGNATURE;
    }

    return CRYPTO_OK;
}
