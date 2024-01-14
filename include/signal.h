#pragma once 

#include "list.h"
#include "sche.h"
/* https://man7.org/linux/man-pages/man7/signal.7.html */
#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4
#define SIGTRAP     5
#define SIGABRT     6
#define SIGIOT      6
#define SIGBUS      7
#define SIGFPE      8
#define SIGKILL     9
#define SIGUSR1     10
#define SIGSEGV     11
#define SIGUSR2     12
#define SIGPIPE     13
#define SIGALRM     14
#define SIGTERM     15
#define SIGSTKFLT   16
#define SIGCHLD     17
#define SIGCONT     18
#define SIGSTOP     19
#define SIGTSTP     20
#define SIGTTIN     21
#define SIGTTOU     22
#define SIGURG      23
#define SIGXCPU     24
#define SIGXFSZ     25
#define SIGVTALRM   26
#define SIGPROF     27
#define SIGWINCH    28
#define SIGIO       29
#define SIGPOLL     29
#define SIGPWR      30
#define SIGSYS      31
#define SIGUNUSED   31
#define NSIG		32

typedef void(*sighandler_t)();

typedef struct signode {
    list_node_t node;
    unsigned int signum;
} signode_t;

typedef struct sigaction {
    sighandler_t handler;
    /* 1 for kernel */
    unsigned int flag;
} sigaction_t;

typedef struct sigtable {
    struct sigaction table[NSIG];
} sigtable_t;


void syscall_sigreturn(trapframe_t *regs);
void syscall_signal(trapframe_t *_, unsigned int signal, sighandler_t handler);
void syscall_sigkill(trapframe_t *_, unsigned long pid);
void handle_signal(trapframe_t *regs);
sigtable_t *sigaction_create();
