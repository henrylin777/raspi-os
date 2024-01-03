#include "uart.h"
#include "shell.h"

void main()
{
    // set up serial console
    uart_init();
    shell_daemon();
}
