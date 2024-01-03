#include "sche.h"
#include "arm.h"
#include "malloc.h"
#include "uart.h"

unsigned long TID_COUNT = 10;

/* add into wait queue and wait to be recycled */
void kthread_fini() {
    prempt_disable();
    task_struct *current = get_current_proc();
    sche_del_task(current);
    sche_add_wait_queue(current);
    current->state = PS_ZOMBIE;
    prempt_enable();
    schedule_task();
    /* never reach */
}

/* create a thread */
task_struct *kthread_create(void *task, void *data, int flag) {
    prempt_disable();
    task_struct *thread = (task_struct *)kmalloc(sizeof(task_struct));
    thread->tid = TID_COUNT++;
    thread->state = PS_INIT;
    thread->preemptable = 0; /* disable preemption until thread is already in run/wait queue */
    thread->regs.sp = kmalloc(STACK_SIZE) + STACK_SIZE; /* allocate kernel stack */
    thread->kstack = (unsigned long) thread->regs.sp;

    /* file */
    for(int i = 0; i < THREAD_MAX_FD; i++)
        thread->fdtable[i].vnode = 0;
    thread->numfd = 0;
    /* signal */
    thread->sigtable = sigaction_create();
    list_init(&thread->pending);
    if (flag) {
        thread->regs.x19 = (unsigned long) task;
        thread->regs.x20 = (unsigned long) data;
        thread->regs.x22 = 0x14; /* for debug */
        thread->regs.x30 = (unsigned long) ret_from_fork;   /* lr */
        
    }
    prempt_enable();
    return thread;
}

