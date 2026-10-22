#include "uart.h"

int main(void)
{
    uart_init();

    uart_puts("\n[app] running unconfirmed\n");

    for (;;) {

    }
}
