#include "uart.h"

void main()
{
    // set up serial console
    uart_init();
    
    // say hello
    uart_send_string("Hello World!\n");
    
    // echo everything back
    while(1) {
        uart_send_char(uart_read_char());
    }
}
