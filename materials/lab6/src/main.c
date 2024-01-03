#include "uart.h"
#include "mmu.h"
#include "utils.h"
#include "kthread.h"
#include "malloc.h"

#define current read_sysreg(tpidr_el1);

/* move to user mode */
// void move_to_user_mode() {
    
//     task_struct *cur = get_current_proc();
//     unsigned long stack = (unsigned long) kmalloc(STACK_SIZE) + STACK_SIZE; /* allocate user stack */
//     cur->regs.sp_el0 = (void *)stack;        /* backup user stack */
//     asm volatile ("msr elr_el1, %0"::"r"((unsigned long) user_start));
//     asm volatile ("msr sp_el0, %0"::"r"(stack));
//     // memzero((unsigned long)&task->regs, sizeof(trapframe_t));
//     /* go to ret_to_user */
// }

void func() {
    while(1) {
        int r = 100000000; while(r--) {};
        task_struct *cur = (task_struct *)current;
        // mem_print((unsigned long) cur);
        uart_send_string("[child thread ");
        uart_send_int(cur->tid);
        uart_send_string("] working...\r\n");
        // schedule_task();
    }
    kthread_fini();
}

void main()
{
    mem_init();
    // mmu_init();
    uart_init();
    
    uart_send_string("type 'help' for more information\r\n");
    DEBUG("SCTLR_EL1: ", read_sysreg(SCTLR_EL1), 0);
    kthread_create(func, (void *)0x13, 1);
    uart_send_string("debug\r\n");
    while(1) {}
}
