#include "uart.h"
#include "bootloader.h"

void main()
{
    // set up serial console
    uart_init();
    // say hello
	int s=500;
	while(s--){
		asm volatile("nop");
	}
	load_image();
}
