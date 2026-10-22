#ifndef IMAGE_COPY_H
#define IMAGE_COPY_H

#include "flash_ops.h"

#include <stdint.h>

typedef enum {
    IMAGE_COPY_OK = 0,
    IMAGE_COPY_ERR_PARAM,
    IMAGE_COPY_ERR_IO,
} image_copy_status_t;

image_copy_status_t image_copy_run(const flash_ops_t *ops, uint32_t dst, uint32_t src, uint32_t len);

#endif
