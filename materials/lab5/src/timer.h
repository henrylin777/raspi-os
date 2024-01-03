
#include "list.h"

#define disable_timer_irq_el0() asm volatile("msr cntp_ctl_el0,%0"::"r"(0));
#define enable_timer_irq_el0() asm volatile("msr cntp_ctl_el0,%0"::"r"(1));

typedef void (*tnode_handler_t)();

typedef struct time_node {
    tnode_handler_t handler;      /* the function to call when the timer expires */
    void *data;             /* data to be passed to the callback */
    unsigned long exp;     /* the time at which the timer will expire */
    list_node_t node;
} tnode_t;

void timer_init(unsigned long period);
void set_timeout(char *message, unsigned long seconds);
void handle_timer_irq();
unsigned long load_status();
void restore_status(unsigned long daif);
