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
#define SYS_SIGNAL      8
#define SYS_SIGKILL     9
#define SYS_PRINT_ADDR  10
#define SYS_FILE_OPEN   11
#define SYS_FILE_CLOSE  12
#define SYS_FILE_WRITE  13
#define SYS_FILE_READ   14
#define SYS_FILE_MKDIR  15
#define SYS_FILE_MOUNT  16
#define SYS_FILE_CHDIR  17


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

/* register signal handler */
void signal(int signal, void(*handler)) {
    syscall(SYS_SIGNAL, (void *)(unsigned long)signal, handler, 0, 0, 0, 0);
}

/* send a signal to kill process with tid */
void sigkill(int pid) {
    syscall(SYS_SIGKILL, (void *)(unsigned long)pid, 0, 0, 0, 0, 0);
}


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

/* open a file and return the file descriptor */
int open(char *fpath) {
    return (int) syscall(SYS_FILE_OPEN, (void *)fpath, 0, 0, 0, 0, 0);
}

int read(unsigned long fd, void *buf, unsigned long len) {
    return (int) syscall(SYS_FILE_READ, (void *)fd, buf, (void *)len, 0, 0, 0);
}

int write(unsigned long fd, void *buf, unsigned long len) {
    return (int) syscall(SYS_FILE_WRITE, (void *)fd, buf, (void *)len, 0, 0, 0);
}

void user_start(void) {
    printf("[User] syscall printf ok\r\n");
    char *fpath = "/initramfs/file1.txt";

    int fd = open(fpath);
    if (fd < 0) {
        // printf("fail to open file %s", fpath);
        while(1){};
    };
    // char buf[64];
    // int res = read((unsigned long)fd, buf, 8);
    // printf("read %d bytes\r\n", res);
    // printf("buf: %s\r\n", buf);

    int uartfd = open("/dev/uart");
    write(uartfd, "hello", 5);
    while(1){};

}
