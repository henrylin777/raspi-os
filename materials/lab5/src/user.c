#include <stddef.h>
#include <stdarg.h>
#include "exception.h"
#include "utils.h"

#define SYS_GETPID      0
#define SYS_UART_RECV   1
#define SYS_UART_WRITE  2
#define SYS_EXEC        3
#define SYS_FORK        4
#define SYS_EXIT        5
#define SYS_MBOX_CALL   6
#define SYS_KILL_PID    7
<<<<<<< HEAD
<<<<<<< HEAD
=======
#define SYS_SIGNAL      8
#define SYS_SIGKILL     9
#define SYS_PRINT_ADDR  10
>>>>>>> 30df1f8 (implement POSIX signal in lab5)
=======
#define SYS_SIGNAL      8
#define SYS_KILL        9
#define SYS_PRINT_ADDR  10
>>>>>>> 5257f88 (clean up lab5)

#define delay(sec) {                  \
    int r = sec;                      \
    while(r--) {asm volatile("nop");} \
    }

/* syscall */
unsigned long syscall(unsigned long opcode, void *x0, void *x1, void *x2, void *x3, void *x4, void *x5)
{
    unsigned long result;
    asm volatile (
        "ldr x8, %0\n"
        "ldr x0, %1\n"
        "ldr x1, %2\n"
        "ldr x2, %3\n"
        "ldr x3, %4\n"
        "ldr x4, %5\n"
        "ldr x5, %6\n"
        "svc 0\n"
        :: "m" (opcode), "m" (x0), "m" (x1), 
           "m" (x2), "m" (x3), "m" (x4), "m" (x5)
    );
    asm volatile ("str x0, %0\n": "=m" (result));

    return result;
}

/* system call to get processor id */
int getpid() {
    return (int) syscall(SYS_GETPID, 0, 0, 0, 0, 0, 0);
    while(1){};
}

/* system call to get size of bytes */
void uart_read(char buf[], size_t size) {
    syscall(SYS_UART_RECV, (void *)buf, (void *)size, 0, 0, 0, 0);
}

/* Poll a syscall to write one char to serial port 
 * for now @size is ignored
 */
void uart_write(const char *buf, unsigned long size) {
    syscall(SYS_UART_WRITE, (void *)buf, (void *)size, 0, 0, 0, 0);
}

void uart_addr(unsigned long addr) {
    syscall(SYS_PRINT_ADDR, (void *)addr, 0, 0, 0, 0, 0);
}

/* execute program */
void exec(const char *name, char *const argv[]) {
    syscall(SYS_EXEC, (void *)name, (void *)argv, 0, 0, 0, 0);
}

/* system call to fork a child thread 
 * return 0 if it is child thread, otherwise would be parent thread 
*/
int fork(void) {
    return (int)syscall(SYS_FORK, 0, 0, (void *)1, 0, 0, 0);
}

void exit(void) {
    syscall(SYS_EXIT, 0, 0, 0, 0, 0, 0);
}

void mbox_call(unsigned char ch, unsigned int *mbox) {
    syscall(SYS_MBOX_CALL, (void *)(unsigned long)ch, mbox, 0, 0, 0, 0);
}

void kill_pid(int pid) {
    syscall(SYS_KILL_PID, (void *)(unsigned long)pid, 0, 0, 0, 0, 0);
}

<<<<<<< HEAD
<<<<<<< HEAD
=======
/* register signal handler */
void signal(int signal, void(*handler)) {
    syscall(SYS_SIGNAL, (void *)(unsigned long)signal, handler, 0, 0, 0, 0);
}

/* send a signal to kill process with tid */
void sigkill(int pid) {
    syscall(SYS_SIGKILL, (void *)(unsigned long)pid, 0, 0, 0, 0, 0);
}

>>>>>>> 30df1f8 (implement POSIX signal in lab5)
=======
/* default signal handler */
void signal(int signal, void(*handler)) {
    return;
}

void kill(int pid, int signal) {
    return;
}

>>>>>>> 5257f88 (clean up lab5)

/* for now we only support %c, %s, and %d format */
void printf(char *format, ...) {
    char c;
    const char *str;
    unsigned long ld;
    char buf[10];
    va_list args;
    va_start(args, format);

    for (; *format; format++) {
        if (*format != '%') {
            uart_write(format, 0);
            continue;
        }
        format++;
        switch (*format) {
            case 'c':
                c = va_arg(args, int);
                uart_write((char *)&c, 1);
                break;
            case 'd':
                ld = va_arg(args, unsigned long);
                int2string(ld, &buf[0]);
                for (int i = 0; buf[i] != '\0'; i++)
                    uart_write(&buf[i], 0);
                break;
            case 's':
                str = va_arg(args, char *);
                for (int i = 0; str[i] != '\0'; i++)
                    uart_write(&str[i], 0);
                break;
            case 'x':
                ld = va_arg(args, unsigned int);
                uart_addr(ld);
                    
                break;
            default:
                break;
        }

    }
}

void show_stack(void) {
    unsigned long sp;
    asm volatile (
        "mov x9, sp\n"
        "str x9, %0\n"
        : "=m" (sp)
    );
    printf("[User] Stack: %d\r\n", sp);
}

void mykill(void) {
    printf("in the regsitered signal handler\r\n");
    exit();
}

void user_start(void) {
    printf("[User] syscall printf ok\r\n");
    int pid, ret, cnt;
    cnt = 1;
    pid = getpid();
    printf("[User] syscall getpid ok\r\n");
    printf("[User] pid: %d\r\n", pid);
<<<<<<< HEAD
<<<<<<< HEAD
    pid = fork();
    if (pid) {
        printf("[User] exit parent thread\r\n");
=======
    ret = fork();
    if (ret) {
        printf("[ thread %d ] this is parent thread, child pid: %d\r\n", pid, ret);
        printf("[ thread %d ] exit\r\n", pid);
>>>>>>> 5257f88 (clean up lab5)
        exit();
=======
    ret = fork();
    if (ret) {
        printf("[ thread %d ] regsiter signal handler at %x\r\n", pid, (unsigned long)mykill);
        signal(9, mykill);
        while(1) {
            printf("[ thread %d ] this is parent thread, child pid: %d\r\n", pid, ret);
            printf("[ thread %d ] send signal to kill this process %d\r\n", pid, pid);
            sigkill(pid);
            while(1){};
        }
>>>>>>> 30df1f8 (implement POSIX signal in lab5)
    } else {
        long long sp;
        asm volatile("mov %0, sp" : "=r"(sp));
        pid = getpid();
        printf("[ thread %d ] cnt: %d, ptr: %x, sp: %x\r\n", pid, cnt, &cnt, sp);
        ++cnt;
        // printf("[ thread %d ] try to fork another child thread\r\n", pid);
        if ((ret = fork()) != 0) {
            pid = getpid();
            asm volatile("mov %0, sp" : "=r"(sp));
            printf("[ thread %d ] cnt: %d, ptr: %x, sp: %x\r\n", pid, cnt, &cnt, sp);
        } else {
            pid = getpid();
            while( cnt < 5) {
                delay(40000000);
                asm volatile("mov %0, sp" : "=r"(sp));
                printf("[ thread %d ] cnt: %d, ptr: %x, sp: %x\r\n", pid, cnt, &cnt, sp);
                ++cnt;
            }
        }
        printf("[ thread %d ] exit\r\n", pid);
        exit();
    }
    return;
}
