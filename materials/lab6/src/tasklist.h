typedef void (*handler_t)();
typedef struct task {
    struct task *prev;
    struct task *next;
    handler_t handler;   /* callback function of interrupt */
    void *data;          /* parameter of hander function */
    unsigned long prio;  /* the priority of handler */
} task_t;

void remove_task(task_t **head, task_t *task);
void insert_task(task_t **head, task_t *task);
void execute_task(task_t **head);
task_t *create_task(void *data, unsigned long prio, handler_t callback);
