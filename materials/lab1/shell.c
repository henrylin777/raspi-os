#include "uart.h"
#include "mailbox.h"
#include "reboot.h"

#define BUFFER_SIZE 256

int strmatch(char *input, char *cmd)
{
    while ((*input == *cmd) && *input) {
        input++;
        cmd++;
    }
    return ( !(*input) && !(*cmd)) ? 1 : 0;
}


void shell_read_cmd(char *buffer)
{
    char *ptr = buffer;
    char data;
    while (1) {
        data = uart_read_char();
        /* to show what we type for each char */
        uart_send_char(data);
        if (data == '\n') {
            *ptr = '\0';
            uart_send_char('\r');
            break;
        }
        *ptr++ = data;
    }
    return;
}

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

void shell_daemon()
{
    char buffer[BUFFER_SIZE];
    // char *pbuffer = buffer;
    uart_send_string("Wellcome to the simple shell\n");
    uart_send_string("Type 'help' for more information\n");
    /*for the bug of U+FFFD in the first character */
    uart_read_char();
    while(1) {
        uart_send_string("# ");
        shell_read_cmd(buffer);

        if (strmatch(buffer, "hello")) {
            uart_send_string("Hello world!\n");

        } else if (strmatch(buffer, "help")) {
            uart_send_string("help    :Show all commands\n");
            uart_send_string("hello   :Print Hello world!\n");
            uart_send_string("info    :Show hardware information\n");
            uart_send_string("reboot  :Reboot device\n");

        } else if (strmatch(buffer, "info")) {
            show_hardware_info();
        } else if (strmatch(buffer, "reboot")) {
            uart_send_string("rebooting...\n");
            reset(1000);
            break;
        } else {
            uart_send_string("unknown command '");
            uart_send_string(buffer);
            uart_send_string("'\n");
        }
    }

}
