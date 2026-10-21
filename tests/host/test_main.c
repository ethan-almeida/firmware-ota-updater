#include "unity.h"

extern void run_crc32_tests(void);
extern void run_flash_tests(void);
extern void run_image_tests(void);
extern void run_config_tests(void);

void crc32_test_set_up(void);
void flash_test_set_up(void);
void image_test_set_up(void);
void config_test_set_up(void);

void setUp(void)
{
    crc32_test_set_up();
    flash_test_set_up();
    image_test_set_up();
    config_test_set_up();
}

void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    run_crc32_tests();
    run_flash_tests();
    run_image_tests();
    run_config_tests();
    return UNITY_END();
}
