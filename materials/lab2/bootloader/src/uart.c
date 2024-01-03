#include "uart.h"


void uart_init()
{
    register unsigned int r;
    /* map UART to GPIO pins 
     * UART tx/rx at GPIO14, GPIO15 in alt5, and GPFSEL1 controls gpio pin[19:10]
     * (p.104, p.102)
     */
    r =* GPFSEL1;
    r&=~((7<<12)|(7<<15)); /* clear gpio14, gpio15 */
    r|=(2<<12)|(2<<15);    /* set gpio14 and 15 to 010 which is alt5 */
    *GPFSEL1 = r;       

    *GPPUD = 0;

    r=150; while(r--) { asm volatile("nop"); }
    /* GPIO control 54 pins, GPPUDCLK0 controls pin[31:0] and GPPUDCLK1 controls pin[53:32]
     * so set bit14, bit15 in GPPUDCLK0 to enable gpio14, gpio15
     */
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        /* flush GPIO setup */ 

    r=1000; while(r--) { asm volatile("nop"); }

    *AUX_ENABLE |=1;    /* enable mini uart */  
    *AUX_MU_CNTL = 0;   /* (p.17) disable tx/rx to avoid data transmission in initialization process */
    *AUX_MU_IER = 0;    
    *AUX_MU_LCR = 3;    /* (p.14) make UART works in 8-bit mode */
    *AUX_MU_MCR = 0;    /* disable auto flow control */
    *AUX_MU_BAUD = 270; /* set BAUD rate to 115200(transmit speed) */
    *AUX_MU_IIR = 0xc6; /* disable interrupt */   
    *AUX_MU_CNTL = 3;   /* enable tx/rx */ 
    
}    
    
void uart_send_char(char c) {
    /* wait until we can send */
    do {asm volatile("nop");} while (!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = (unsigned int) c;
}


char uart_read_char() {
    /* wait until something is in the buffer */
    do {asm volatile("nop");} while (!(*AUX_MU_LSR & 0x01));
    /* read it and return */
    char c = (char) (*AUX_MU_IO);
    /* convert carriage return to newline */
    return (c == '\r') ? '\n' : c;
}


void uart_send_string(char *s) {
    while(*s) {
        /* convert newline to carriage return + newline */
        if(*s == '\n')
            uart_send_char('\r');
        uart_send_char(*s++);
    }
}

/* Send binary data in hexadecimal format */
void uart_binary_to_hex(unsigned int d) {
    unsigned int n;
    int c;
    uart_send_string("0x");
    for(c = 28; c >= 0; c -= 4) {
        /* get highset 4 bits */
        n = (d >> c)& 0xF;
        /*
         * 0x37: 0011 0111
         * 0x30: 0011 0000
         */
        n += n > 9 ? 0x37 : 0x30;
        uart_send_char(n);
    }
}
