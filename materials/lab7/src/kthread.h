#include "sche.h"

void kthread_fini();
task_struct *kthread_create(void *func, void *data, int flag);
task_struct *kthread_create_task();
