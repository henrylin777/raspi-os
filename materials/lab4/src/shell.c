#include "uart.h"
#include "utils.h"
#include "allocater.h"
#include "exception.h"
#include "malloc.h"
#include "slub.h"
#include "buddy.h"

#define BUFFER_SIZE 256
#define N_ARGS 16


void read_command_async(char *command)
{
    char data;
    int idx;
    while (1) {
        idx = 0;
        uart_async_send("# ");
        while (1) {
            if(!uart_async_read(&data))
                continue;
            uart_async_send(&data);
            if (data == '\n') {
                command[idx] = '\0';
                break;
            };
            command[idx] = data;
            idx++;
        }
        if (*command) return;
    }
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

void print_usage() {
    uart_send_string("malloc <size>      : allocate a memory space and return a pointer\n");
    uart_send_string("free <addr>        : free out a pointer\n");
    uart_send_string("print <addr>       : print the value of address\n");
    uart_send_string("buddy info         : show the current status of buddy system\n");
    uart_send_string("buddy <size>       : allocate a page of size\n");
    uart_send_string("slub info          : show the current status of slub allocator\n");
    uart_send_string("set <value>        : write value into memory address \n");
}

void shell_main()
{
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char argument[BUFFER_SIZE];
    uart_enable_interrupt();
    mem_init();
    uart_send_string("type 'help' for more information\n");
    unsigned long *addr = (void *)0;
    while(1) {
        read_command_async(buffer);
        if(!parse_command(buffer, command, argument))
            continue;
        if (strmatch(command, "malloc")) {
            unsigned long size = decstr_to_int(argument);
            addr = (unsigned long *)kmalloc(size);
            DEBUG("the address is ", (unsigned long)addr, 0);
        } else if (strmatch(command, "help")) {
            print_usage();
        } else if (strmatch(command, "free")) {
            unsigned long addr = hexstr_to_int(argument);
            kfree((void *)addr);
        } else if (strmatch(command, "set")) {
            unsigned long val = decstr_to_int(argument);
            *addr = val;
            uart_send_string("write ");
            uart_send_int(val);
            DEBUG(" at ", (unsigned long)addr, 0);
        } else if (strmatch(command, "print")) {
            unsigned long addr = hexstr_to_int(argument);
            mem_print(addr);
        } else if (strmatch(command, "buddy")) {
            if (strmatch(argument, "info")) {
                buddy_info();
            } else {
                unsigned long size = decstr_to_int(argument);
                addr = (unsigned long *)buddy_alloc_page(size);
                DEBUG("got a page at ", (unsigned long)addr, 0);
            }
        } else if (strmatch(command, "slub")) {
            if (strmatch(argument, "info")) {
                slub_info();
            } else {
                print_usage();
            }
        } else {
            uart_send_string("unknown command '");
            uart_send_string(command);
            uart_send_string("'\n");
        }
    }
}
