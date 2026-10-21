#include "uart.h"

int main(void)
{
    uart_init();

    uart_puts("\n[app] hello from the application image\n");

    for (;;) {

    }
}
