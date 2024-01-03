#pragma once

#include "list.h"
#include "mmzone.h"
#include "utils.h"
#include "arm.h"
#include "signal.h"
#include "vfs.h"

#define STACK_SIZE (PAGE_SIZE * 2)
#define KSTACK_SIZE (PAGE_SIZE * 2)
#define PS_INIT   1 /* for the status of thread that just finish initialization but not in run queue */
#define PS_RUN    2 /* for the status of thread that finish initialization and in run queue */
#define PS_SLEEP  3 /* for the status of thread that in wait queue and wait for signal */
#define PS_ZOMBIE 4 /* for the status of child thread that exit the code but not recycled by parent */
#define PS_DEAD   5 /* for the status of parent thread that exit the code but not recycled */

#define TRAPFRAME_OFFSET 152 // offset of callee-saved registers 

#define THREAD_MAX_FD 8

typedef struct thread {
    trapframe_t regs;
    /* file*/
    vnode_t *workdir;
    int numfd; /* number of opened file in file descriptor table */
    file_t fdtable[THREAD_MAX_FD]; /* file descriptor table */

    unsigned long state;
    unsigned long tid;  /* thread id */
    unsigned long kstack; /* kernel stack */
    int preemptable; /* indicate if current thread is preemptable or not */
    int child; /* indicate if this thread has child thread */
    struct sigtable *sigtable;
    list_node_t pending; /* a list of pending signals */
    list_node_t node;
} task_struct;


/* set the current process
 * @val : address to current task_struct
 * current always pointer to the registers of the current process
 */
#define set_current_proc(val) {write_sysreg(tpidr_el1, val);}


task_struct *get_current_proc();
void sche_init();
void sche_add_task(task_struct *task);
void sche_del_task(task_struct *task);
task_struct *sche_get_task(unsigned long pid);
void sche_add_wait_queue(task_struct *task);
void schedule_task();
/* save thread context registers into trapframe and load thread context from trapframe 
 * jump to `lr` register and execute 
 */
extern void switch_to(unsigned long prev, unsigned long next);
/* in schedule.s */
void ret_from_fork(void);
task_struct *get_current_proc();
void kill_zombies();
void prempt_enable();
void prempt_disable();
