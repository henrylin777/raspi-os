#include "tasklist.h"
#include "allocater.h"
#include "exception.h"
#include "uart.h"

void tasklist_init(task_t *head)
{
    head->prev = head;
    head->next = head;
}

/* insert task into task list */
void insert_task(task_t **head, task_t *task) 
{
    if (!*head) {
        *head = task;
        return;
    }
    task->next = (*head)->next;
    (*head)->next->prev = task;
    task->prev = *head;
    (*head)->next = task;

}

/* remove task into task list */
void remove_task(task_t **head, task_t *task) 
{
    if (*head == task) {
        *head = 0;
        return;
    }
    task_t *tmp = task;
    tmp->prev->next = tmp->next;
    tmp->next->prev = tmp->prev;
    // free(tmp);
}

/* create a task node */
task_t *create_task(void *data, unsigned long prio, handler_t callback)
{
    task_t *new_task = simple_malloc(sizeof(task_t));
    new_task->data = data;
    new_task->prio = prio;
    new_task->handler = callback;
    return new_task;
}

/* execute all the tasks in task list  */
void execute_task(task_t **head)
{   
    while (*head) {
        task_t *task = *head;    
        task->handler();
        remove_task(head, task);

    }
    enable_interrupt();
    return;
}

