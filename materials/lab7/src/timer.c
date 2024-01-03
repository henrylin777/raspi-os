
#include "timer.h"
#include "uart.h"
#include "utils.h"
#include "exception.h"
#include "slub.h"
#include "sche.h"
#include "malloc.h"

list_node_t *TIMER_LIST_HEAD = 0;
unsigned long PERIOD = 1;

/* enable and poll el1 timer interrupt after exp_time */
void enable_timer_interrupt_el1()
{                                                      
        unsigned long cntfrq, exp_time;
        asm volatile("msr cntp_ctl_el0, %0"::"r"(0x1));
        asm volatile("mrs %0, cntpct_el0":"=r"(exp_time));
	    asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq));
        exp_time += (cntfrq * 2);
        asm volatile("msr cntp_cval_el0, %0"::"r"(exp_time));
}


/* callback function: print out the elapsed seconds after booting.
 * Note that this is a callback function of timer irq.
 */
void print_elapsed_time(void *data) {
    char *message = (char *) data;
	unsigned long current_time, cntfrq;
    asm volatile("mrs %0, cntpct_el0" : "=r"(current_time));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq));
    unsigned long seconds = current_time / cntfrq;

	uart_send_string("Timeout message: ");
	uart_send_string(message);
	uart_send_string(" occurs at: ");
	uart_binary_to_hex(seconds);
	uart_send_string("\n");
}

/* main function to handle timer irq */
void handle_timer_irq() {
    uart_send_string("[ timer interrupt ]\r\n");
    disable_timer_irq_el0();
    disable_interrupt();

    // /* calculate the expired time */
	unsigned long current_time, cntfrq, exp_time;
	asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
	asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq));
	exp_time = current_time + PERIOD;
    /* update the expired time */
    asm volatile("msr cntp_cval_el0, %0"::"r"(exp_time));

    if (list_empty(TIMER_LIST_HEAD) != 0) {
        tnode_t *timer = list_first_entry(TIMER_LIST_HEAD, tnode_t, node);
        list_remove(&timer->node);
        timer->handler(timer->data);
    }
    task_struct *cur = get_current_proc();
    if (cur->preemptable) {
        DEBUG("thread ", cur->tid, 1);
        uart_send_string("preemptable\r\n");
        schedule_task();
    }
    enable_interrupt();
    enable_timer_irq_el0();
};


unsigned long load_status() {
    unsigned long res = 0;
    asm volatile ("mrs %0, DAIF":"=r"(res));
    return res;

}

void restore_status(unsigned long daif) {
    asm volatile("msr DAIF, %0"::"r"(daif));
}

/* insert node into timer list in ascending order */
void timer_insert(list_node_t *node, unsigned long time) {
    tnode_t *timer;
    list_node_t *next = TIMER_LIST_HEAD->next;
    while(next != TIMER_LIST_HEAD) {
        timer = list_entry(next, tnode_t, node);
        if (timer->exp <= time)
            next = next->next;
    }
    list_insert(next->prev, node);
}

/* create a timer node */
int timer_create(char* message, unsigned long seconds, tnode_handler_t callback) {
    tnode_t *timer = (tnode_t *)kmalloc(sizeof(tnode_t));
    
    if (!timer) {
        uart_send_string("timer_insert: failed to simple malloc\n");
        return 0;
    }

	/* calculate the expired time */
	unsigned long current_time, cntfrq, exp_time;
	asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
	asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq));
	exp_time = current_time + seconds * cntfrq;

    timer->exp = exp_time;
    timer->handler = callback;
    timer->data = message;
    timer_insert(&timer->node, exp_time);
    uart_send_string("create_timer finished\n");
    return 1;
}



/* print the `message` after seconds via cpu timer */
void set_timeout(char *message, unsigned long sec)
{
	if (!TIMER_LIST_HEAD) {
		/* enable core timer interrupt */
		unsigned int *addr = (unsigned int *)CORE0_TIMER_IRQ_CTRL;
		*addr = 2;
	};
	timer_create(message, sec, print_elapsed_time);
}

/* initialize periodic timer */
void timer_init(unsigned long seconds) {
    
    list_node_t *TIMER_LIST_HEAD = (list_node_t *)kmalloc(sizeof(list_node_t));
    list_init(TIMER_LIST_HEAD);

	unsigned long current_time, cntfrq, exp_time;
	asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
	asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq));
    PERIOD = seconds * cntfrq;
	exp_time = current_time + PERIOD;

    /* update the expired time */
    asm volatile("msr cntp_cval_el0, %0"::"r"(exp_time));


    /* enable core0 timer interrupt */
    put32(0x40000040, 2);
}

void timer_reset() {
    /* todo */ 
}


void timer_destroy() {
    /* todo */ 
}
