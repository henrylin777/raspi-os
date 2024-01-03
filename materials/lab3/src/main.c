#include "uart.h"
#include "shell.h"

void main()
{
    uart_init();
    unsigned long el = 0;
    asm volatile ("mrs %0, CurrentEL":"=r"(el));
	asm volatile("mov %0, sp"::"r"(el));
	uart_send_string("Current stack pointer address: ");
	uart_binary_to_hex(el);
	uart_send_string("\n");
    shell_main();

}
