#include "uart.h"
#include "utils.h"
#include "cpio.h"
#include "exception.h"
#include "timer.h"
#include "malloc.h"
#include "sche.h"
#include "user.h"
#include "exec.h"
#include "kthread.h"
#include "vfs.h"
#include "tmpfs.h"
#include "uartfs.h"
#include "sdhost.h"
#include "fat32.h"
#include "kprintf.h"

#define BUFFER_SIZE 128
#define unused(x) (void)(x)
void idle() {
    while(1) {
        delay(100000000);
        uart_send_string("[idle thread] kill zombies\r\n");
        kill_zombies();
        uart_send_string("[idle thread] do schedule_task\r\n");
        schedule_task();
    }
}


/* move to user mode */
void move_to_user_mode() {

    task_struct *cur = get_current_proc();
    unsigned long stack = (unsigned long) kmalloc(STACK_SIZE) + STACK_SIZE; /* allocate user stack */
    cur->regs.sp = (void *)stack;        /* backup user stack */
    asm volatile ("msr elr_el1, %0"::"r"((unsigned long) user_start));
    asm volatile ("msr sp_el0, %0"::"r"(stack));
    // memzero((unsigned long)&task->regs, sizeof(trapframe_t));
    /* go to ret_to_user */
}

/* parse buffer and split out command and arguements */
int parse_command(char *buffer, char *command, char *argument)
{
    if (!buffer)
        return 0;
    char *pbuffer = buffer;
    char *pcmd = command;
    char *parg = argument;

    for (; *pbuffer != ' ' && *pbuffer; pcmd++, pbuffer++) {
        *pcmd = *pbuffer;
    }
    *pcmd = '\0';
    if (!*pbuffer)
        return 1;
    pbuffer++;
    /* for now we only parse command with one argument */
    for (; *pbuffer; parg++, pbuffer++)
        *parg = *pbuffer;
    *parg = '\0';
    return 1;
}

void read_command(char *command)
{
    char data;
    int idx;
    while (1) {
        idx = 0;
        uart_send_string("# ");
        while (1) {
            data = uart_read_char();
            if (data == '\r') {
                uart_send_string("\r\n");
                command[idx] = '\0';
                break;
            };
            uart_send_string(&data);
            command[idx] = data;
            idx++;
        }
        if (*command) return;
    }
}

void shell_main(){
   
    mem_init();
    sche_init();
    vfs_init();
    sd_init();
    fs_t *tmpfs = tmpfs_init();
    vfs_register_fs(tmpfs);
    vfs_init_rootmount(tmpfs);

    fs_t *fat32 = fat32_init();
    vfs_register_fs(fat32);
    vfs_mkdir("boot");
    vfs_mount("/boot", "fat32fs");
    // vfs_umount("boot");
    // vfs_chdir("boot");
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char argument[BUFFER_SIZE];
    int fd = -1;
    // fd = vfs_create("/boot/file3.txt");
    // vfs_write(fd, "abc", 3);
    // vfs_close(fd);
    // while(1){};
    // fd = vfs_open("/abc/file3.txt");
    // char buf[64];
    // if (vfs_read(fd, buf, 64) < 0) {
    //     uart_send_string("vfs_read failed\r\n");
    //     while(1){};
    // }
    // buf[3] = '\0';
    // kprintf("content: %s\r\n", buf);
    // while(1){};
    
    while(1){
        read_command(buffer);
        if(!parse_command(buffer, command, argument)) {
            continue;
        }
        if (strmatch(command, "pwd")) {
            vfs_pwd();
        } else if (strmatch(command, "ls")) {
            vfs_ls();
        } else if (strmatch(command, "cd")) {
            vfs_chdir(argument);
        } else if (strmatch(command, "umount")) {
            vfs_umount(argument);
        } else if (strmatch(command, "mkdir")) {
            vfs_mkdir(argument);
        } else if (strmatch(command, "write")) {
            int len = strlen(argument);
            vfs_write(fd, argument, len);
        } else if (strmatch(command, "create")) {
            fd = vfs_create(argument);
            kprintf("fd: %d\r\n", fd, 1);
        } else if (strmatch(command, "open")) {
            fd = vfs_open(argument);
           kprintf("fd: %d\r\n", fd, 1);
        } else if (strmatch(command, "read")) {
            char buf[32];
            if (vfs_read(fd, buf, 32) < 0) {
                uart_send_string("vfs_read failed\r\n");
                continue; 
            }
            kprintf("file content: %s\r\n", buf);
        } else {
            kprintf("unknown command '%s'\r\n", command);
        };
    };
    // int uart = vfs_open("/dev/uart");
    // vfs_write(uart, "hello\r\n", 7);
    // while(1){};


    /* test system call */
    // task_struct *task;
    // task = kthread_create(move_to_user_mode, (void *)0x13, 1);
    // sche_add_task(task);
    // schedule_task();
    
}
