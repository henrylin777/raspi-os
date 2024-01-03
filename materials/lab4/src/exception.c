#include "uart.h"
#include "exception.h"
#include "gpio.h"
#include "timer.h"
#include "allocater.h"
#include "utils.h"
#include "tasklist.h"

#define CNTPSIRQ_BIT_POSITION 0x02
#define AUXINIT_BIT_POSTION 1<<29

task_t *TASK_HEAD = 0;


void print_exception_info(){
	/* print the content of reg `spsr_el1` */ 
	unsigned long long spsr_el1 = 0;
	asm volatile("mrs %0, spsr_el1":"=r"(spsr_el1));
	uart_send_string("spsr_el1: "); /* should print out 0x3c0 */
	uart_binary_to_hex(spsr_el1);
	uart_send_string("\n");

	/* print the content of reg `elr_el1` */ 
	unsigned long long elr_el1 = 0;
	asm volatile("mrs %0, elr_el1":"=r"(elr_el1));
	uart_send_string("elr_el1: "); /* should print out 0x20000 */
	uart_binary_to_hex(elr_el1);
	uart_send_string("\n");
	
    /* print the content of reg `esr_el1` */ 
	unsigned long long esr_el1 = 0;
	asm volatile("mrs %0, esr_el1":"=r"(esr_el1));
	uart_binary_to_hex(esr_el1);
	uart_send_string("\n");

	unsigned ec = (esr_el1 >> 26) & 0x3F; /* 0x3F: 0011 1111 */
	uart_send_string("ec: ");
	uart_binary_to_hex(ec);
	uart_send_string("\n");
    // while(1){};
    // asm volatile("msr spsr_el1, %0" ::"r"(0x3c0));
    // asm volatile("msr elr_el1, lr");
    asm volatile("ret");
}

/* handler function when uart tx interrupt(send data to serial port) is triggered */
void uart_send_handler() {
    char *ptr = get_wbuffer_addr();
    uart_send_byte(*ptr);
    disable_tx_interrupt();
}

/* callback function of uart rx interrupt */
void uart_read_handler() {
    disable_rx_interrupt();
    char *rbuffer = get_rbuffer_addr();
    char data = uart_read_byte();
    *rbuffer = data;
}

/* This function will be executed if an exception is taken (switch from el0 to el1)
 * for now this function just prints out the three registers.
 */
void handle_exception() {
    task_t *task = create_task(0, 1, print_exception_info);
    disable_interrupt();
    insert_task(&TASK_HEAD, task);
    enable_interrupt();
    disable_interrupt();
    execute_task(&TASK_HEAD);
}


/* handle exception request */
void handle_irq() {
    disable_interrupt();
	unsigned int irq_pending1 = mmio_read(IRQ_PENDING_1);
	unsigned int core0_interrupt_source = mmio_read(CORE0_INTERRUPT_SOURCE);	
	unsigned int iir = mmio_read(AUX_MU_IIR);
    /* handle timer interrupt */
	if (core0_interrupt_source & CNTPSIRQ_BIT_POSITION) { 
        handle_timer_irq();
    };
    
    /* handle uart rx/tx interrupt */
    if (irq_pending1 & AUXINIT_BIT_POSTION) {
        if ((iir & 0x06) == 0x04) { /* receive data */
            task_t *task = create_task(0, 3, uart_read_handler);
            insert_task(&TASK_HEAD, task);
        } else if ((iir & 0x06) == 0x02) { /* transmit hold reg empty */ 
            task_t *task = create_task(0, 3, uart_send_handler);
            insert_task(&TASK_HEAD, task);
        }
    }
    execute_task(&TASK_HEAD);
}



void show_current_el() {
    unsigned long el = 0;
    asm volatile ("mrs %0, CurrentEL":"=r"(el));
	uart_send_string("Current Exception Level: ");
	uart_binary_to_hex(((el >> 2) & 0x3)); // CurrentEL store el at el[3:2]
	uart_send_string("\n");
}
