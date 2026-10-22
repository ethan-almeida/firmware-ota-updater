#ifndef IMAGE_H
#define IMAGE_H

#include <stddef.h>
#include <stdint.h>

#define IMAGE_MAGIC 0x3141544Fu
#define IMAGE_HEADER_VERSION 1u

#define IMAGE_PAYLOAD_OFFSET 512u

#define IMAGE_SHA256_LEN    32u
#define IMAGE_SIGNATURE_LEN 64u

#define IMAGE_PAYLOAD_MAX (255u * 1024u)

typedef struct {
    uint32_t magic;
    uint32_t header_version;
    uint32_t firmware_version;
    uint32_t image_size;
    uint32_t load_address;
    uint32_t entry_address;
    uint8_t sha256[IMAGE_SHA256_LEN];
    uint8_t signature[IMAGE_SIGNATURE_LEN];
    uint32_t header_crc;
} image_header_t;

typedef enum {
    IMAGE_OK = 0,
    IMAGE_ERR_TRUNCATED,
    IMAGE_ERR_MAGIC,
    IMAGE_ERR_HEADER_VERSION,
    IMAGE_ERR_SIZE,
    IMAGE_ERR_ADDRESS,
    IMAGE_ERR_CRC,
} image_status_t;

image_status_t image_header_decode(const uint8_t *raw, size_t len, image_header_t *out);

#endif
