#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST_LEN 32u
#define SHA256_BLOCK_LEN  64u

typedef struct {
    uint32_t state[8];
    uint64_t length;
    uint32_t buffer_len;
    uint8_t buffer[SHA256_BLOCK_LEN];
} sha256_ctx_t;

void sha256_init(sha256_ctx_t *ctx);
void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, size_t len);
void sha256_final(sha256_ctx_t *ctx, uint8_t digest[SHA256_DIGEST_LEN]);
void sha256(const uint8_t *data, size_t len, uint8_t digest[SHA256_DIGEST_LEN]);

#endif
