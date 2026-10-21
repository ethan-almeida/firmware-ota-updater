#include "uart.h"

int main(void)
{
    uart_init();

    uart_puts("\n[bootloader] ready\n");

    for (;;) {
        
    }
}
