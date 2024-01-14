#include <timer.h>
#include <uart.h>
#include <utils.h>
#include <exception.h>
#include <slub.h>
#include <sche.h>
#include <malloc.h>

list_node_t *TIMER_LIST_HEAD;

void update_exp_time()
{                                                      
        unsigned long cntfrq, exp_time;
        asm volatile("mrs %0, cntpct_el0":"=r"(exp_time));
	    asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq));
        exp_time += cntfrq;
        asm volatile("msr cntp_cval_el0, %0"::"r"(exp_time));
}

void print_elapsed_time(void *data) {
	uart_send_string("Timer message: ");
	uart_send_string((char *)data);
	uart_send_string("\r\n");
}

/* main function to handle timer irq */
void handle_timer_irq() {
    disable_timer_irq_el0();
    disable_interrupt();

    if (list_empty(TIMER_LIST_HEAD) != 1) {
	    unsigned long cur_time;
	    asm volatile("mrs %0, cntpct_el0":"=r"(cur_time));
        tnode_t *timer = list_first_entry(TIMER_LIST_HEAD, tnode_t, node);
        if (cur_time > timer->exp) {
            list_remove(&timer->node);
            timer->handler(timer->data);
        }
    }
    // task_struct *cur = get_current_proc();
    // if (cur->preemptable) {
    //     DEBUG("thread ", cur->tid, 1);
    //     uart_send_string("preemptable\r\n");
    //     schedule_task();
    // }
    update_exp_time();
    enable_timer_irq_el0();
    enable_interrupt();
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
void timer_insert(list_node_t *new_node, unsigned long time) {
    tnode_t *timer;
    list_node_t *cur = TIMER_LIST_HEAD;
    list_for_each(cur, TIMER_LIST_HEAD) {
        timer = list_entry(cur, tnode_t, node);
        if (timer->exp <= time)
            break;
    }
    list_insert(cur->prev, new_node);
}

/* create a timer node */
int timer_create(char* message, unsigned long seconds, tnode_handler_t callback) 
{
    tnode_t *timer = (tnode_t *)kmalloc(sizeof(tnode_t));
    if (!timer) {
        uart_send_string("timer_insert: failed to malloc\r\n");
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
    return 1;
}



/* print the `message` after seconds via cpu timer */
void set_timeout(char *message, unsigned long sec)
{
	timer_create(message, sec, print_elapsed_time);
}

/* initialize periodic timer */
void timer_init() {
    TIMER_LIST_HEAD = (list_node_t *)kmalloc(sizeof(list_node_t));
    list_init(TIMER_LIST_HEAD);

	unsigned long cntfrq, exp_time;
	asm volatile("mrs %0, cntpct_el0":"=r"(exp_time));
	asm volatile("mrs %0, cntfrq_el0":"=r"(cntfrq));
	exp_time += cntfrq;
    asm volatile("msr cntp_cval_el0, %0"::"r"(exp_time));

    /* enable core0 timer interrupt */
    enable_timer_irq_el0();
    enable_core0_timer();
}

void timer_reset() {
    /* todo */ 
}


void timer_destroy() {
    /* todo */ 
}
