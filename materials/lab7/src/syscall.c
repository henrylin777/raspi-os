#include "syscall.h"
#include "arm.h"
#include "utils.h"
#include "uart.h"
#include "sche.h"
#include "malloc.h"
#include "kthread.h"
#include "exception.h"
#include "signal.h"
#include "vfs.h"

#define UNUSED(x) (void)(x)

#define KSTACK_VARIABLE(x)                      \
    (void *)((unsigned long)x -                        \
             (unsigned long)current->kstack +    \
             (unsigned long)child->kstack)


void syscall_getpid(trapframe_t *regs);
void syscall_uart_read(trapframe_t *_, char *buffer, int n);
void syscall_uart_write(trapframe_t *_, char *c);
void syscall_exec(trapframe_t *_, char *fname, char *argv[]);
void syscall_fork(trapframe_t *regs);
void syscall_exit(trapframe_t *_);
void syscall_mbox(trapframe_t *_, unsigned char ch, unsigned int *mbox);
void syscall_kill(trapframe_t *_, unsigned long pid);
void syscall_print_addr(trapframe_t *_, unsigned long addr);

syscall_t syscall_table[] = {
    (syscall_t) syscall_getpid,
    (syscall_t) syscall_uart_read,
    (syscall_t) syscall_uart_write,
    (syscall_t) syscall_exec,
    (syscall_t) syscall_fork,
    (syscall_t) syscall_exit,
    (syscall_t) syscall_kill, /* unfinished */
    (syscall_t) syscall_kill,
    (syscall_t) syscall_signal,
    (syscall_t) syscall_sigkill,
    (syscall_t) syscall_print_addr,
    (syscall_t) syscall_open,
    (syscall_t) syscall_close,
    (syscall_t) syscall_write,
    (syscall_t) syscall_read,
    (syscall_t) syscall_mkdir,
    (syscall_t) syscall_mount,
    (syscall_t) syscall_chdir,
};

/* save registers into trapframe */
void save_regs(unsigned long *sp, trapframe_t *regs) {

    unsigned long elr_el1, spsr_el1;
    /* x0 ~ x30 (31), sp (32),  */
    elr_el1 = *(sp + (32 * 8));
    spsr_el1 = *(sp + (33 * 8));
    regs->elr_el1 = (unsigned long *)elr_el1;
    regs->spsr_el1 = (unsigned long *)spsr_el1;
};

/*
#define save_regs(sp)                       \
    asm volatile (                          \
        "stp x25, x26, [%x0, 16 * 16]\n"    \
        "msr elr_el1, x25 \n"               \
        "msr spsr_el1, x26 \n"              \
        : : "r" (sp)               \
    );
*/
 

void ret_from_child_fork();

void copy_regs(trapframe_t *src, trapframe_t *dst) {
    // dst->x0 = src->x0;
    // dst->x1 = src->x1;
    // dst->x2 = src->x2;
    // dst->x3 = src->x3;
    // dst->x4 = src->x4;
    // dst->x5 = src->x5;
    // dst->x6 = src->x6;
    // dst->x7 = src->x7;
    // dst->x8 = src->x8;
    // dst->x9 = src->x9;
    // dst->x10 = src->x10;
    // dst->x11 = src->x11;
    // dst->x12 = src->x12;
    // dst->x13 = src->x13;
    // dst->x14 = src->x14;
    // dst->x15 = src->x15;
    // dst->x16 = src->x16;
    // dst->x17 = src->x17;
    // dst->x18 = src->x18;
    dst->x19 = src->x19;
    dst->x20 = src->x20;
    dst->x21 = src->x21;
    dst->x22 = src->x22;
    dst->x23 = src->x23;
    dst->x24 = src->x24;
    dst->x25 = src->x25;
    dst->x26 = src->x26;
    dst->x27 = src->x27;
    dst->x28 = src->x28;
    dst->x29 = src->x29;
    dst->x30 = src->x30;
    // dst->sp = src->sp;
    // dst->sp_el0 = src->sp_el0;
    // dst->elr_el1 = src->elr_el1;
    // dst->spsr_el1 = src->spsr_el1;
}

void syscall_getpid(trapframe_t *regs) {
    task_struct *cur;
    asm volatile ("mrs %0, tpidr_el1":"=r"(cur));
    regs->x0 = cur->tid;
}

void syscall_uart_read(trapframe_t *_, char *buffer, int n) {
    UNUSED(_);
    while(n--)
        *buffer++ = uart_read_char();
}

void syscall_uart_write(trapframe_t *_, char *c) {
    UNUSED(_);
    uart_send_char(*c);
}

void syscall_exec(trapframe_t *_, char *fname, char *argv[]) {
    uart_send_string("in syscall_exec\r\n");
    while(1){};
    /* TBD */
    UNUSED(_);
    UNUSED(fname);
    UNUSED(argv);
    return;
}


void user_ret_from_fork() {
    asm volatile("ret");
}


/* copy data section and initialize a new stack, setup signal and parent process, setup system registers 
 * and return to user program
 * return value:
 * On success, the pid of child process is returned in parent, and 0 is returned in child
 * On failure, -1 is returned in parent and no child process is created  
 */
void syscall_fork(trapframe_t *regs) {
    task_struct *current;
    current = get_current_proc();
    // current->regs.x2 = 1;
    uart_send_string("[ thread ");
    uart_send_int(current->tid);
    uart_send_string(" ] in the fork syscall\r\n");
    // DEBUG("regs->x2: ", regs->x2, 0);

    if (current->child == 0) { /* child thread */
        // uart_send_string("[ thread ");
        // uart_send_int(current->tid);
        // uart_send_string(" ] goto SYSCALL_FORK_END\r\n");
        goto SYSCALL_FORK_END;
    }
    task_struct *child = kthread_create(0, 0, 0);
    // if (!child) {
    //     regs->x0 = -1;
    //     return;
    // }
    child->regs.sp = kmalloc(STACK_SIZE) + STACK_SIZE; /* allocate user stack */
    // uart_send_string("[ thread ");
    // uart_send_int(current->tid);
    // uart_send_string(" ] fork a  the fork syscall\r\n");
    // uart_send_int(child->tid);
    // uart_send_string("\r\n");
    /* copy whole stack and register data */
    memcopy((void *)child->kstack - STACK_SIZE, (void *)current->kstack - STACK_SIZE, STACK_SIZE);
    memcopy(child->regs.sp - STACK_SIZE, current->regs.sp - STACK_SIZE, STACK_SIZE);
    copy_regs(regs, &child->regs);
    /* calculate the correct offset of sp from kernel stack */
    unsigned long tmp;
    asm volatile("mov %0, sp":"=r"(tmp));
    child->regs.sp = KSTACK_VARIABLE(tmp);
    asm volatile("mov %0, x29":"=r"(tmp));
    child->regs.x29 = (unsigned long) KSTACK_VARIABLE(tmp);
    

    /* https://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html */
    child->regs.x30 = (unsigned long) &&SYSCALL_FORK_END;
    // child->regs.x30 = (unsigned long) user_ret_from_fork;
    
    /* set return value of fork system call */
    regs->x0 = child->tid;
    child->regs.x0 = 0;
    sche_add_task(child);
    child->preemptable = 1;
    uart_send_string("[ thread ");
    uart_send_int(current->tid);
    uart_send_string(" ] fork child thread with tid ");
    uart_send_int(child->tid);
    uart_send_string("\r\n");
    current->child = 1;
SYSCALL_FORK_END:
    current = get_current_proc();
    uart_send_string("[ thread ");
    uart_send_int(current->tid);
    uart_send_string(" ] end of fork syscall\r\n");
    enable_interrupt();
    return;
}

void syscall_exit(trapframe_t *_)
{
    UNUSED(_);
    kthread_fini();
    // Never reach
}

void syscall_kill(trapframe_t *_, unsigned long pid) {
    UNUSED(_);
    task_struct *task;
    task = get_current_proc();
    if (task->tid == pid) {
        kthread_fini();
        return;
    }
    task = sche_get_task(pid);

    if (!task) {
        return;
    }
    sche_del_task(task);
    task->state = PS_DEAD;
    sche_add_wait_queue(task); /* put them into wait queue and wait to be recycled */

}

void syscall_print_addr(trapframe_t *_, unsigned long addr) {
    UNUSED(_);
    uart_binary_to_hex(addr);
}

void handle_syscall(trapframe_t *regs) {
    unsigned long opcode = regs->x8;
    // if (opcode > ARRAY_SIZE(syscall_table)) {
    //     return;
    // }
    (syscall_table[opcode])(regs, regs->x0, regs->x1, regs->x2, regs->x3, regs->x4, regs->x5);
}
