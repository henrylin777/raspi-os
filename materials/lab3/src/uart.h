/* Auxilary mini UART registers */
#define AUX_ENABLE      ((volatile unsigned int*)(MMIO_BASE+0x00215004))
#define AUX_MU_IO       ((volatile unsigned int*)(MMIO_BASE+0x00215040))
/* primarily used to enable interrupts. 
 * If this bit is set, the interrupt line is asserted whenever the transmit FIFO is empty.
 * If this bit is clear no transmit interrupts are generated.
 */
#define AUX_MU_IER      ((volatile unsigned int*)(MMIO_BASE+0x00215044))
/* showing the interrupt status */
#define AUX_MU_IIR      ((volatile unsigned int*)(MMIO_BASE+0x00215048))
#define AUX_MU_LCR      ((volatile unsigned int*)(MMIO_BASE+0x0021504C))
#define AUX_MU_MCR      ((volatile unsigned int*)(MMIO_BASE+0x00215050))
#define AUX_MU_LSR      ((volatile unsigned int*)(MMIO_BASE+0x00215054))
#define AUX_MU_MSR      ((volatile unsigned int*)(MMIO_BASE+0x00215058))
#define AUX_MU_SCRATCH  ((volatile unsigned int*)(MMIO_BASE+0x0021505C))
#define AUX_MU_CNTL     ((volatile unsigned int*)(MMIO_BASE+0x00215060))
#define AUX_MU_STAT     ((volatile unsigned int*)(MMIO_BASE+0x00215064))
#define AUX_MU_BAUD     ((volatile unsigned int*)(MMIO_BASE+0x00215068))

#define RX_INTERRUPT_BIT    0x01
#define TX_INTERRUPT_BIT    0x02
#define AUXINIT_BIT_POSTION 1<<29


/* enable rx interrupt and second-level interrupt controller */
#define uart_enable_interrupt() \
    mmio_write(ENABLE_IRQS_1, mmio_read(ENABLE_IRQS_1) | AUXINIT_BIT_POSTION); \
    mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) | 0x1)

#define uart_disable_interrupt() \
    mmio_write(ENABLE_IRQS_1, mmio_read(ENABLE_IRQS_1) & ~AUXINIT_BIT_POSTION)

#define disable_tx_interrupt() \
    mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) & ~0x2)

#define disable_rx_interrupt() \
    mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) & ~0x1)

#define enable_rx_interrupt() \
    mmio_write(AUX_MU_IER, mmio_read(AUX_MU_IER) | 0x1)

#define enable_tx_interrupt() \
    unsigned int mask = mmio_read(AUX_MU_IER); \
    mask |= 0x2; \
    mmio_write(AUX_MU_IER, mask);

#define UART_BUFFER_SIZE 512
extern char UART_READ_BUFFER[UART_BUFFER_SIZE];
extern char UART_WRITE_BUFFER[UART_BUFFER_SIZE];


void uart_init();
void uart_send_char(unsigned int c);
char uart_read_char();
void uart_send_string(char *s);
void uart_binary_to_hex(unsigned int);
void uart_read_handler();
void uart_send_handler();
int uart_async_read(char *buffer);
void uart_async_send(char *data);
void uart_send_byte(char c);
char uart_read_byte();
char *get_wbuffer_addr();
char *get_rbuffer_addr();
