#include "image_copy.h"

#include <stddef.h>

static image_copy_status_t erase_span(const flash_ops_t *ops, uint32_t dst, uint32_t len)
{
    uint32_t addr = dst;
    uint32_t end = dst + len;

    while (addr < end) {
        uint32_t base;
        uint32_t size;

        if (!ops->sector_span(ops->ctx, addr, &base, &size) || size == 0u || base > addr ||
            base + size <= addr) {
            return IMAGE_COPY_ERR_PARAM;
        }

        if (ops->erase_sector(ops->ctx, addr) != FLASH_OPS_OK) {
            return IMAGE_COPY_ERR_IO;
        }

        addr = base + size;
    }

    return IMAGE_COPY_OK;
}

image_copy_status_t image_copy_run(const flash_ops_t *ops, uint32_t dst, uint32_t src, uint32_t len)
{
    if (ops == NULL || ops->read32 == NULL || ops->program32 == NULL ||
        ops->erase_sector == NULL || ops->sector_span == NULL ||
        (dst & 3u) != 0u || (src & 3u) != 0u || (len > 0u && dst + len < dst)) {
        return IMAGE_COPY_ERR_PARAM;
    }

    if (len == 0u) {
        return IMAGE_COPY_OK;
    }

    image_copy_status_t status = erase_span(ops, dst, len);
    if (status != IMAGE_COPY_OK) {
        return status;
    }

    for (uint32_t offset = 0; offset < len; offset += 4u) {
        uint32_t word = ops->read32(ops->ctx, src + offset);
        uint32_t remaining = len - offset;

        if (remaining < 4u) {
            uint32_t keep = (1u << (8u * remaining)) - 1u;
            word |= ~keep;
        }

        if (ops->program32(ops->ctx, dst + offset, word) != FLASH_OPS_OK) {
            return IMAGE_COPY_ERR_IO;
        }
    }

    return IMAGE_COPY_OK;
}
