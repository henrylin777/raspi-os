#include "arm.h"
#include "utils.h"

typedef void (*syscall_t)();

void handle_syscall(trapframe_t *regs);
