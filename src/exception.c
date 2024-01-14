#include <uart.h>
#include <exception.h>
#include <gpio.h>
#include <timer.h>
#include <utils.h>
#include <arm.h>
#include <syscall.h>
#include <malloc.h>
#include <sche.h>

#define CNTPSIRQ_BIT_POSITION 0x02
#define AUXINIT_BIT_POSTION 1<<29

list_node_t *IRQ_LIST_HEAD = 0;

typedef struct {
    unsigned int iss:25, // Instruction specific syndrome
                 il:1,   // Instruction length bit
                 ec:6;   // Exception class
} esr_el1_t;

static inline void restore_interrupt(unsigned int daif) {
    write_sysreg(DAIF, daif);
}

void show_trapframe(trapframe_t *regs) {
        uart_send_string("trapframe content\n");
    DEBUG("x0:  ", regs->x0, 0);
    DEBUG("x1:  ", regs->x1, 0);
    DEBUG("x2:  ", regs->x2, 0);
    DEBUG("x3:  ", regs->x3, 0);
    DEBUG("x4:  ", regs->x4, 0);
    DEBUG("x5:  ", regs->x5, 0);
    DEBUG("x6:  ", regs->x6, 0);
    DEBUG("x7:  ", regs->x7, 0);
    DEBUG("x8:  ", regs->x8, 0);
    DEBUG("x9:  ", regs->x9, 0);
    DEBUG("x10: ", regs->x10, 0);
    DEBUG("x11: ", regs->x11, 0);
    DEBUG("x12: ", regs->x12, 0);
    DEBUG("x13: ", regs->x13, 0);
    // DEBUG("x14: ", regs->x14, 0);
    // DEBUG("x15: ", regs->x15, 0);
    // DEBUG("x16: ", regs->x16, 0);
    // DEBUG("x17: ", regs->x17, 0);
    // DEBUG("x18: ", regs->x18, 0);
    // DEBUG("x19: ", regs->x19, 0);
    // DEBUG("x20: ", regs->x20, 0);
    DEBUG("x21: ", regs->x21, 0);
    // DEBUG("x22: ", regs->x22, 0);
    // DEBUG("x23: ", regs->x23, 0);
    DEBUG("x24: ", regs->x24, 0);
    // DEBUG("x25: ", regs->x25, 0);
    // DEBUG("x26: ", regs->x26, 0);
    // DEBUG("x27: ", regs->x27, 0);
    // DEBUG("x28: ", regs->x28, 0);
    DEBUG("x29: ", regs->x29, 0);
    DEBUG("x30: ", regs->x30, 0);
    DEBUG("sp:", (unsigned long) regs->sp, 0);
    DEBUG("elr_el1:", (unsigned long) regs->elr_el1, 0);
    DEBUG("spsr_el1:", (unsigned long) regs->spsr_el1, 0);
}

void show_task_info(task_struct *task) {
    DEBUG("tid: ", task->tid, 1);
    show_trapframe(&task->regs);
}



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


void irq_init() {
    IRQ_LIST_HEAD = (list_node_t *)kmalloc(sizeof(list_node_t));
    list_init(IRQ_LIST_HEAD);
}

void irq_insert(list_node_t *node) {
    list_add_tail(node, IRQ_LIST_HEAD);
}

void irq_remove(list_node_t *node) {
    list_remove(node);
}

void irq_create(irq_handler_t handler, void *data, unsigned long prio) {
    irq_t *task = (irq_t *)kmalloc(sizeof(irq_t));
    task->data = data;
    task->handler = handler;
    task->prio = prio;
    disable_interrupt();
    irq_insert(&task->node);
    enable_interrupt();
}


void irq_execute() {
    disable_interrupt();
    list_node_t *cur = IRQ_LIST_HEAD->next;
    irq_t *task;
    task = list_entry(cur, irq_t, node);
    while(cur != IRQ_LIST_HEAD) {
        task->handler(task->data);
        cur = cur->next;
        irq_remove(cur->prev);
        task = list_entry(cur, irq_t, node);
    }
    enable_interrupt();
}

/* This function will be executed if an exception is taken (switch from el0 to el1)
 * for now this function just prints out the three registers.
 */
void handle_exception() {
    irq_create(print_exception_info, 0, 1);
    irq_execute();
}


/* handle exception request */
void handle_irq() {
    disable_interrupt();
	unsigned int irq_pending1 = mmio_read(IRQ_PENDING_1);
	unsigned int core0_interrupt_source = mmio_read(CORE0_INTERRUPT_SOURCE);	
	unsigned int iir = mmio_read(AUX_MU_IIR);
	if (core0_interrupt_source & CNTPSIRQ_BIT_POSITION) { 
        handle_timer_irq();
        return;
    };
    
    /* handle uart rx/tx interrupt */
    if (irq_pending1 & AUXINIT_BIT_POSTION) {
        if ((iir & 0x06) == 0x04) { /* receive data */
            /* handle immediately */
            uart_read_handler();
        } else if ((iir & 0x06) == 0x02) { /* transmit hold reg empty */ 
            /* handle immediately */
            uart_send_handler();
        }
    }
    irq_execute();
}


void el0_sync_handler(struct trapframe *regs, unsigned int syn) {
    disable_interrupt();
    esr_el1_t *esr_el1 = (esr_el1_t *)&syn;
    // DEBUG("esr_el1->iss: ", (unsigned long) esr_el1->iss, 0);
    // DEBUG("esr_el1->il: ", (unsigned long) esr_el1->il, 0);
    // DEBUG("esr_el1->ec: ", (unsigned long) esr_el1->ec, 0);
    if (esr_el1->ec == EC_SVC_64) {
        // uart_send_string("debug\r\n");
        handle_syscall(regs);
    } else {
        show_trapframe(regs);
        uart_send_string("[ERROR] unknown exception\r\n");
        while(1){};
    }
    // uart_send_string("end of el0_sync_handler\r\n");
    enable_interrupt();
}

void show_current_el() {
    unsigned long el = 0;
    asm volatile ("mrs %0, CurrentEL":"=r"(el));
	uart_send_string("Current Exception Level: ");
	uart_binary_to_hex(((el >> 2) & 0x3)); // CurrentEL store el at el[3:2]
	uart_send_string("\n");
}

void exit_to_user_mode(trapframe_t *regs) {
    handle_signal(regs);
    // uart_send_string("end of exit_to_user_mode\r\n");
}
