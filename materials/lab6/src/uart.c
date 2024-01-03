#include "gpio.h"
#include "utils.h"
#include "uart.h"

char UART_READ_BUFFER[UART_BUFFER_SIZE];
char UART_WRITE_BUFFER[UART_BUFFER_SIZE];

char *get_wbuffer_addr() {
    return UART_WRITE_BUFFER;
}

char *get_rbuffer_addr() {
    return UART_READ_BUFFER;
}


void uart_init()
{
    register unsigned int r;

    /* initialize UART */
    *AUX_ENABLE |=1;       // enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;
    *AUX_MU_LCR = 3;       // 8 bits
    *AUX_MU_MCR = 0;
    *AUX_MU_IER = 0;
    *AUX_MU_IIR = 0xc6;    // disable interrupts
    *AUX_MU_BAUD = 270;    // 115200 baud
    /* map UART1 to GPIO pins */
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    r|=(2<<12)|(2<<15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0;            // enable pins 14 and 15
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    *AUX_MU_CNTL = 3;      // enable tx, rx
    
}

void uart_send_char(unsigned int c) {
    /* wait until we can send */
    do {asm volatile("nop");} while (!(*AUX_MU_LSR & 0x20));
    /* write the character to the buffer */
    *AUX_MU_IO = c;
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
        /* convert newline("\n") to carriage return + newline("\r\n") */
        if(*s == '\n')
            uart_send_char('\r');
        uart_send_char(*s++);
    }
}

/* Send binary data in hexadecimal format */
void uart_binary_to_hex(unsigned long d) {
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

/* Send binary data in hexadecimal format */
void uart_print_addr(unsigned long addr) {
    unsigned long n;
    int c;
    uart_send_string("0x");
    for(c = 28; c >= 0; c -= 4) {
        /* get highset 4 bits */
        n = (addr >> c)& 0xF; 
        /* 
         * 0x37: 0011 0111
         * 0x30: 0011 0000 
         */
        n += n > 9 ? 0x37 : 0x30; 
        uart_send_char(n);
    }
}


inline void uart_send_byte(char c) {
    mmio_write(AUX_MU_IO, (unsigned int) c);
}

inline char uart_read_byte() {
    char c = (char) mmio_read(AUX_MU_IO);
    return (c == '\r') ? '\n' : c;
}

/* non-blocking uart read 
 * If there is new data then write them into buffer and return 1
 * otherwise return 0 immediately
 */
int uart_async_read(char *buffer) {
    if(!UART_READ_BUFFER[0])
        return 0;
    *buffer = UART_READ_BUFFER[0];
    UART_READ_BUFFER[0] = 0;
    enable_rx_interrupt(); /* waiting to read */
    return 1;
}

/* non-blocking uart send. */
void uart_async_send(char *data) {
    while (*data) {
        UART_WRITE_BUFFER[0] = *data;
        enable_tx_interrupt(); /* poll a interrupt request and goto interrupt handler */
        data++;
    }
    disable_tx_interrupt();
}

void DEBUG(char *format, unsigned long data, int type) {
    uart_send_string(format);
    if(type) {
        char buffer[10];
        int2string(data, &buffer[0]);
        uart_send_string(buffer);
    } else {
        uart_binary_to_hex(data);
    }
    uart_send_string("\r\n");
}

void uart_send_int(unsigned long data) {
    char b[10];
    int2string(data, &b[0]);
    uart_send_string(b);
}
