
#include "timer.h"
#include "uart.h"
#include "utils.h"
#include "allocater.h"
#include "exception.h"

struct irqnode *TIMER_HEAD = 0;

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
    if (TIMER_HEAD == 0) return;

    disable_timer_irq_el0();
	unsigned long current_time;
	asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
 
    TIMER_HEAD->handler(TIMER_HEAD->data); /* execture callback function (print_elapsed_time) */

    /* remove node from timer list, for now we just unlink the tail node without releasing the memory space */
    TIMER_HEAD = TIMER_HEAD->next;
    
    /* update hardware timer of polling the next interrupt */
    if(TIMER_HEAD != 0) {
        asm volatile("msr cntp_cval_el0, %0"::"r"(TIMER_HEAD->exp));
        enable_timer_irq_el0();
    }
    return;
}



/* create a irqnode and insert into timer_list */
int timer_insert(char* message, unsigned long seconds, handler_t callback) {

    irqnode_t *node = (irqnode_t *) simple_malloc(sizeof(irqnode_t));
    if (!node) {
        uart_send_string("timer_insert: failed to simple malloc\n");
        return 0;
    }
    
	char *copymsg = strcopy(message);
	if(!copymsg) {
        uart_send_string("timer_insert: failed to strcopy\n");
        return 0;
    };

	/* calculate the expired time */
	unsigned long current_time, cntfrq, exp_time;
	asm volatile("mrs %0, cntpct_el0":"=r"(current_time));
	asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq));
	exp_time = current_time + seconds * cntfrq;

    node->exp = exp_time;
    node->handler = callback;
    node->data = copymsg;

    /* insert new timer into timer list */
    if (!TIMER_HEAD) {
        node->next = (void *) 0;
        TIMER_HEAD = node;
    } else {
        irqnode_t **ptr = &TIMER_HEAD;
        while (*ptr != 0 && (*ptr)->exp <= exp_time ) {
            ptr = &(*ptr)->next;
        };
        node->next = *ptr;
        *ptr = node;
        // uart_send_string("debug\n");
    }
    uart_send_string("create_timer finished\n");
    return 1;
}


/* print the `message` after seconds via cpu timer */
void set_timeout(char *message, unsigned long seconds)
{
	if (!TIMER_HEAD) {
		/* enable core timer interrupt */
		unsigned int *addr = (unsigned int *)CORE0_TIMER_IRQ_CTRL;
		*addr = 2;
	};
	timer_insert(message, seconds, print_elapsed_time);
}
