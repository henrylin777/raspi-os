#include "uart.h"
#include "shell.h"

void main()
{
    uart_init();
	int s = 10000; while(s--){asm volatile("nop");};
    shell_main();

}
