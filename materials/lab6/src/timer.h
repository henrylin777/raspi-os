
#define disable_timer_irq_el0() asm volatile("msr cntp_ctl_el0,%0"::"r"(0));
#define enable_timer_irq_el0() asm volatile("msr cntp_ctl_el0,%0"::"r"(1));

typedef void (*handler_t)();
typedef struct irqnode {
    struct irqnode* next;   /* next timer in the list */ 
    handler_t handler;      /* the function to call when the timer expires */
    void *data;             /* data to be passed to the callback */
    unsigned long exp;     /* the time at which the timer will expire */
} irqnode_t;

/* the head of timer_list */
extern struct irqnode *TIMER_HEAD;

void set_timeout(char *message, unsigned long seconds);
void handle_timer_irq();
void set_schedule_timer(unsigned long seconds);
unsigned long load_status();
void restore_status(unsigned long daif);
