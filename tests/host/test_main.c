#include "unity.h"

extern void run_crc32_tests(void);

int main(void)
{
    UNITY_BEGIN();
    run_crc32_tests();
    return UNITY_END();
}
