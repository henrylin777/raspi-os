#include <uart.h>
#include <shell.h>

void main()
{
    uart_init();
    shell_main();

}
