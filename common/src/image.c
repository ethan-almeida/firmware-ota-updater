#include "image.h"

#include "crc32.h"

#include <string.h>

_Static_assert(sizeof(image_header_t) == 124u, "image header layout must be stable");

static image_status_t check_addresses(const image_header_t *header)
{
    if ((header->load_address & 3u) != 0u || (header->entry_address & 3u) != 0u) {
        return IMAGE_ERR_ADDRESS;
    }

    uint32_t end = header->load_address + header->image_size;
    if (end < header->load_address) {
        return IMAGE_ERR_ADDRESS;
    }

    if (header->entry_address < header->load_address || header->entry_address >= end) {
        return IMAGE_ERR_ADDRESS;
    }

    return IMAGE_OK;
}

image_status_t image_header_decode(const uint8_t *raw, size_t len, image_header_t *out)
{
    if (raw == NULL || len < sizeof(image_header_t)) {
        return IMAGE_ERR_TRUNCATED;
    }

    image_header_t header;
    memcpy(&header, raw, sizeof(header));

    uint32_t expected = crc32_calc(raw, sizeof(image_header_t) - sizeof(uint32_t));
    if (expected != header.header_crc) {
        return IMAGE_ERR_CRC;
    }

    if (header.magic != IMAGE_MAGIC) {
        return IMAGE_ERR_MAGIC;
    }

    if (header.header_version != IMAGE_HEADER_VERSION) {
        return IMAGE_ERR_HEADER_VERSION;
    }

    if (header.image_size == 0u || header.image_size > IMAGE_PAYLOAD_MAX) {
        return IMAGE_ERR_SIZE;
    }

    image_status_t status = check_addresses(&header);
    if (status != IMAGE_OK) {
        return status;
    }

    if (out != NULL) {
        *out = header;
    }

    return IMAGE_OK;
}
