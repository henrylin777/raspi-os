#include "arm.h"
#include "signal.h"
#include "malloc.h"
#include "kthread.h"
#include "uart.h"
#include "user.h"
#include "sche.h"

#define UNUSED(x) (void)(x)

struct sigtable *default_sigtable = 0;

void signal_init() {
    return;
}

void sig_ignore() {
    return;
}

void sig_term() {
    kthread_fini();
}


void sigreturn() {
    uart_send_string("in sigreturn\r\n");
    while(1){};
    asm volatile (
        "mov x8, 11\n"
        "svc 0\n"
    );
}

void signal_add(list_node_t *head, unsigned int signum) {
    signode_t *new = (signode_t *)kmalloc(sizeof(signode_t));
    new->signum = signum;
    list_add_tail(&new->node, head);
}

void signal_del(signode_t *signal) {
    // signode_t *tmp = signal;
    list_remove(&signal->node);
    // kfree(tmp);
}

void syscall_sigreturn(trapframe_t *regs) {
    /* restore context */
    char *sp = (char *)regs->sp;
    memcopy((char *)regs, sp, sizeof(trapframe_t));
    /* TODO: mark signal as complete */
    return;
}

/* kill process with tid via posix signal */
void syscall_sigkill(trapframe_t *_, unsigned long pid) {
    // uart_send_string("in syscall_sigkill\r\n");
    UNUSED(_);
    prempt_disable();
    task_struct *task = sche_get_task(pid);
    if (task && task->state == PS_RUN) {
        // DEBUG("add kill signal into thread ", pid, 1);
        signal_add(&task->pending, SIGKILL);
    }
    prempt_enable();
}

/* let user register signal handler for some signal */
void syscall_signal(trapframe_t *_, unsigned int signal, sighandler_t handler) {
    UNUSED(_);
    if (signal > NSIG)
        return;
    task_struct *cur = get_current_proc();
    sigaction_t *sigaction = &cur->sigtable->table[signal];
    sigaction->flag = 0;
    sigaction->handler = handler;  
    
}
/* check if there is pending signal in the current process */
int signal_pending(task_struct *current) {
    return (list_empty(&current->pending) == 0);
}

/* after this function, data in kerenl stack will be transfered to user mode
 * @regs : kernel stack pointer 
 */
void handle_signal(trapframe_t *regs) {
    // uart_send_string("in handle_signal\r\n");
    signode_t *signal = 0;
    sigaction_t *action;

    task_struct *cur = get_current_proc();
    if (signal_pending(cur) == 0)
        return;

    signal = list_first_entry(&cur->pending, signode_t, node);
    uart_send_string("thread ");
    uart_send_int(cur->tid);
    uart_send_string(" handle signal ");
    uart_send_int(signal->signum);
    uart_send_string("\r\n");
    
    action = &cur->sigtable->table[signal->signum];

    if (action->flag) {
        uart_send_string("do it in kernel mode\r\n");
        action->handler();
    } else {
        uart_send_string("do it in user mode\r\n");
        /* reserve space for regsiter context */
        // char *sp = regs->sp_el0 - alignto(sizeof(trapframe_t), 16);
        /* save cpu context on user stack */
        // memcopy(sp, (char *)regs, sizeof(trapframe_t));
        // regs->sp_el0 = sp;

        regs->x0 = signal->signum;
        /* set return address to signal handler, handler will be executed in call eret */
        regs->elr_el1 = action->handler;
        /* set lr to sigreturn, sigreturn will be execute when returning from handler */
        regs->x30 = (unsigned long) sigreturn;

    }
    signal_del(signal);
    uart_send_string("end of handle_signal\r\n");
    
}

sigtable_t *sigaction_create() {
    sigtable_t *sigtable = (sigtable_t *)kmalloc(sizeof(sigtable_t));

    for (int i = 1; i < NSIG; ++i) {
        sigtable->table[i].flag = 1;
        sigtable->table[i].handler = sig_ignore;
    }

    sigtable->table[SIGKILL].handler = sig_term;
    return sigtable;
}
