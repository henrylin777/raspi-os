#include "uart.h"
#include "utils.h"
#include "cpio.h"
#include "exception.h"
#include "timer.h"
#include "malloc.h"
#include "sche.h"
#include "user.h"
#include "exec.h"
#include "kthread.h"

#define current read_sysreg(tpidr_el1);

/* in utils.s */
void memzero(unsigned long addr, unsigned long size);

/* a function for demostrating kernel thread switch */
void func() {
    while(1) {
        delay(100000000);
        task_struct *cur = (task_struct *)current;
        // mem_print((unsigned long) cur);
        uart_send_string("[child thread ");
        uart_send_int(cur->tid);
        uart_send_string("] working...\r\n");
        // schedule_task();
    }
    kthread_fini();
}

/* idle thread */
void idle() {
    while(1) {
        delay(100000000);
        uart_send_string("[idle thread] kill zombies\r\n");
        kill_zombies();
        uart_send_string("[idle thread] do schedule_task\r\n");
        schedule_task(); 
    }
}

/* move to user mode */
void move_to_user_mode() {
    
    task_struct *cur = get_current_proc();
    unsigned long stack = (unsigned long) kmalloc(STACK_SIZE) + STACK_SIZE; /* allocate user stack */
    cur->regs.sp = (void *)stack;        /* backup user stack */
    asm volatile ("msr elr_el1, %0"::"r"((unsigned long) user_start));
    asm volatile ("msr sp_el0, %0"::"r"(stack));
    // memzero((unsigned long)&task->regs, sizeof(trapframe_t));
    /* go to ret_to_user */
}

/* test kernel thread switch and timer interrupt */
void test1(int timer_enable) {
    task_struct *task;
    task = kthread_create((void *)idle, (void *)0x13, 1);
    sche_add_task(task);
    task->preemptable = 1;
    // for (int idx = 0; idx < 3; idx++) {
    //     task = kthread_create(func, (void *)0x13, 1);
    //     sche_add_task(task);
    //     uart_send_string("create a thread\r\n");
    // };
    if (timer_enable) {
        /* TODO: fix timer interrupt */
        timer_init(2);
        enable_interrupt();
    }
    schedule_task();
}

/* test user thread and system call */
void test2() {
    task_struct *task;
    task = kthread_create((void *)idle, (void *)0x13, 1);
    sche_add_task(task);
    task = kthread_create(move_to_user_mode, (void *)0x13, 1);
    sche_add_task(task);
    timer_init(2);
    enable_interrupt();
    schedule_task();
}


void shell_main()
{
    
    uart_enable_interrupt();
    mem_init();
    sche_init();
    show_current_el();
    task_struct *task;
    task = kthread_create((void *)idle, (void *)0x13, 1);
    sche_add_task(task);
    task->preemptable = 1;
    /* task 1 : thread switch */
    // set_schedule_timer(2);
    
    // set_schedule_timer(1);
    
    for (int idx = 0; idx < 3; idx++) {
        task = kthread_create(func, (void *)0x13, 1);
        sche_add_task(task);
        uart_send_string("create a thread\r\n");
    };
    timer_init(1);
    enable_interrupt();
    schedule_task();

    /* task 2 : test if syscall 'printf' and 'get_pid' works correctly */
    // task = kthread_create(move_to_user_mode, (void *)0x13, 1);
    // sche_add_task(task);
    // schedule_task();
    // uart_send_string("syscall test finished\r\n"); /* should be here after user_start */
    // while(1){};
    // cpio_ls();
    // exec_user_prog(&user_start);
    // uart_send_string("end of shell_main\n");
}
