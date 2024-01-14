#include <malloc.h>
#include <mmzone.h>
#include <exec.h>
#include <kthread.h>
#include <sche.h>
#include <cpio.h>
#include <user.h>

/* in schedule.s 
 * set jump address, init user stack, enable interrupt, that's it. 
 */
void enter_el0_run_user_prog();
unsigned long USER_PID = 33;

void exec_syscall() {
    task_struct *task = (task_struct *)kmalloc(sizeof(task_struct));
    unsigned long sp = (unsigned long) kmalloc(STACK_SIZE);
    task->tid = USER_PID;
    task->regs.x0 = (unsigned long) user_start;
    task->regs.sp = (void *)sp + STACK_SIZE;
    task->regs.x30 = (unsigned long) ret_from_fork;
    sche_add_task(task);
}


/* execute the first user program */
void exec_user_prog(char *fname) {
    // int fsize;
    char *fp;
    cpio_find_file(fname, &fp);
    // if (!fsize) {
    //     uart_send_string("exec_user_prog: no such file\r\n");
    //     return;
    // }
    return;    
    /* initialize text section */


    // unsigned long stack = (unsigned long) kmalloc(STACK_SIZE);

    // task_struct *task = kthread_create_task();
    // task->kstack = kmalloc(KSTACK_SIZE);
    // task->regs.sp = (void *)stack;
    // task->regs.sp_el0 = (void *)stack;
    // task->regs.spsr_el1 = 0;
    // task->regs.elr_el1 = (void *)func;

    // sche_add_task(task);
    // task->state = PS_RUN;
    // asm volatile ("msr tpidr_el1, %0"::"r"((unsigned long)task));

    // asm volatile ("msr elr_el1, %0"::"r"((unsigned long)func));
    // asm volatile ("msr sp_el0, %0"::"r"(stack));
    // asm volatile ("msr spsr_el1, %0"::"r"(0));
    // asm volatile ("eret");

    // enter_el0_run_user_prog((unsigned long)func, stack);
    /* User program should call exit() to terminate */ 
}
