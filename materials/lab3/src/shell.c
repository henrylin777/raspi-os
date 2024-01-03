#include "uart.h"
#include "mailbox.h"
#include "reboot.h"
#include "utils.h"
#include "cpio.h"
#include "allocater.h"
#include "devicetree.h"
#include "timer.h"
#include "exception.h"

#define BUFFER_SIZE 256
#define N_ARGS 16

void show_hardware_info()
{
    // if (call_mailbox() == 0) {
    //     uart_send_string("Unable to call mailbox!\n");
    //     return;
    // }
    get_serial_number();
    uart_send_string("Serial number: ");
    uart_binary_to_hex(MAILBOX[6]);
    uart_binary_to_hex(MAILBOX[5]);
    uart_send_string("\r\n");
    get_board_revision();
    uart_send_string("Board revision: ");
    uart_binary_to_hex(MAILBOX[5]);
    uart_send_string("\r\n");
    get_arm_mem();
    uart_send_string("Base address of ARM memory: ");
    uart_binary_to_hex(MAILBOX[5]);
    uart_send_string("\r\n");
    uart_send_string("ARM memory size: ");
    uart_binary_to_hex(MAILBOX[6]);
    uart_send_string("\r\n");

}


void print_usage() 
{
    uart_send_string("help          : Show all commands\n");
    uart_send_string("hello         : Print Hello world!\n");
    uart_send_string("info          : Show hardware information\n");
    uart_send_string("reboot        : Reboot device\n");
    uart_send_string("dfb <device>  : Get the deivce information\n");
}

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

void read_command(char *command)
{
    char data;
    int idx;
    while (1) {
        idx = 0;
        uart_send_string("# ");
        while (1) {
            data = uart_read_char();
            uart_send_string(&data);
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


void shell_main()
{
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char argument[BUFFER_SIZE];
    // char *pbuffer = buffer;
    show_current_el();
    uart_send_string("Wellcome to the simple shell\r\n");
    uart_send_string("Type 'help' for more information\r\n");
    /*for the bug of U+FFFD in the first character */
    // uart_read_char();
    /*TODO: backspacing */
    uart_enable_interrupt();
    while(1) {
        
        read_command_async(buffer);
        if(!parse_command(buffer, command, argument)){
            continue;
        };
        if (strmatch(command, "hello")) {
            uart_send_string("Hello world!\n");
        } else if (strmatch(command, "ls")) {
            cpio_ls();
        } else if (strmatch(command, "cat")) {
            cpio_cat(argument);
        } else if (strmatch(command, "malloc")) {
            unsigned int nchar = strlen(argument);
            unsigned long size = decstr2int(argument, nchar);
            char *space = (char *) simple_malloc(size);
            if (!space) {
                uart_send_string("failed to allocate\n");
                continue;
            }
            /* test */
            test_malloc(space, argument);
        } else if (strmatch(command, "dtb")) {
            traverse_dtb(argument);
        } else if (strmatch(command, "timer")) {
            enable_timer_interrupt_el1();
            set_timeout("hello world3", 3);
            set_timeout("hello world5", 5);
            set_timeout("hello world5", 7);
        } else if (strmatch(command, "help")) {
            print_usage();
        } else if (strmatch(command, "exec")) {
            cpio_exec_prog("func.img");
        } else if (strmatch(command, "info")) {
            show_hardware_info();
        } else if (strmatch(command, "reboot")) {
            uart_send_string("rebooting...\n");
            reset(1000);
            break;
        } else {
            uart_send_string("unknown command '");
            uart_send_string(command);
            uart_send_string("'\n");
        }
    }
}
