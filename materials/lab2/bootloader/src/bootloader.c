
#include "uart.h"
#include "bootloader.h"

void load_image(){
    unsigned int size = 0;
    unsigned char *size_buffer = (unsigned char *) &size;
    uart_send_string("waiting for loading kernel\n");
    for(int i = 0; i < 4; i++) 
	    size_buffer[i] = uart_read_char();
    uart_send_string("size checked\n");

    char *kernel = (char *) 0x80000;
    while(size--) *kernel++ = uart_read_char();

    uart_send_string("kernel loaded\n");
    int s = 30000; while(s--){asm volatile("nop");};
    asm volatile(
       "mov x30, 0x80000;"
       "ret;"
    );

}
