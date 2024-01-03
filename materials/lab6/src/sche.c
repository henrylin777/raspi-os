#include "malloc.h"
#include "sche.h"
#include "list.h"
#include "slub.h"
#include "uart.h"
#include "sche.h"
#include "kthread.h"
#include "exception.h"
#include "timer.h"

// static trapframe_t init_regs = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static task_struct INIT_TASK = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    0, /* state */
    0, /* tid */
    0,
    0,
    0,
    0,
    {0, 0} /* node */
};



list_node_t *run_queue;
list_node_t *wait_queue;

task_struct *get_current_proc() {
    return (task_struct *)read_sysreg(tpidr_el1);
}

void prempt_enable() {
    task_struct *thread = get_current_proc();
    thread->preemptable = 1;
}

void prempt_disable() {
    task_struct *thread = get_current_proc();
    thread->preemptable = 0;
}


// struct kmem_cache *task_allocator;

/* initialize run queue and wait queue */
void sche_init() {
    run_queue = (list_node_t *)kmalloc(sizeof(list_node_t));
    list_init(run_queue);
    wait_queue = (list_node_t *)kmalloc(sizeof(list_node_t));
    list_init(wait_queue);
    INIT_TASK.regs.sp = kmalloc(PAGE_SIZE);
    set_current_proc(&INIT_TASK);

    // slub_register_cache(&task_allocator, sizeof(task_struct));
}

/* add task into run queue and wait to be scheduled */
void sche_add_task(task_struct *task) {
    list_add_tail(&task->node, run_queue);
    task->state = PS_RUN;
}

/* remove task from run queue */
void sche_del_task(task_struct *task) {
    list_remove(&task->node);
}

void sche_add_wait_queue(task_struct *task) {
    list_add_tail(&task->node, wait_queue);
}

void schedule_tail(void) {
    // uart_send_string("in schedule_tail\n");
    prempt_enable();
	enable_timer_irq_el0();
}

unsigned long get_current_context(){
	unsigned long cur;
	asm volatile("mrs %0, tpidr_el1":"=r"(cur));
    return cur;
}

void kill_zombies() {
    if (list_empty(wait_queue))
        return;

    list_node_t *head, *cur, *tmp;
    task_struct *task;
    head = wait_queue;
    cur = wait_queue->next;
    while (cur != head) {
        task = list_entry(cur, task_struct, node);
        if (task->state == PS_ZOMBIE) {
            tmp = cur;
            list_remove(tmp);
        }
        cur = cur->next;
    }
}

/* pick a thread in run queue and execute */
void schedule_task() {
    prempt_disable();
    if (list_empty(run_queue)) {
        uart_send_string("no running task in run queue, return\r\n");
        return;
    }
    task_struct *task, *current;
    task = list_first_entry(run_queue, task_struct, node);
    current = get_current_proc();
    // unsigned long daif = load_status();

    /* FIFO manner */
    list_remove(&task->node);
    list_add_tail(&task->node, run_queue);
    // restore_status(daif);
    prempt_enable();
    set_current_proc(task);

    // prempt_enable();
    enable_interrupt();
    enable_timer_irq_el0();
    switch_to((unsigned long) &current->regs, (unsigned long) &task->regs);
    prempt_enable();
}

task_struct *sche_get_task(unsigned long pid) {
    task_struct *task;
    list_node_t *cur = run_queue;
    do {
        task = list_entry(cur, task_struct, node);
        if (task->tid == pid)
            return task;
        cur = cur->next;
    } while (cur != run_queue);

    return (void *)0;
}

