#include <stdarg.h>
#include <stddef.h>
#include "uart.h"
#include "utils.h"

/* for now we only support %c, %s, and %d format */
void kprintf(char *format, ...) {
    char c;
    const char *str;
    unsigned long ld;
    char buf[10];
    va_list args;
    va_start(args, format);

    for (; *format; format++) {
        if (*format != '%') {
            uart_send_char(*format);
            continue;
        }
        format++;
        switch (*format) {
            case 'c':
                c = va_arg(args, int);
                uart_send_char(c);
                break;
            case 'd':
                ld = va_arg(args, unsigned long);
                int2string(ld, &buf[0]);
                for (int i = 0; buf[i] != '\0'; i++)
                    uart_send_char(buf[i]);
                break;
            case 's':
                str = va_arg(args, char *);
                for (int i = 0; str[i] != '\0'; i++)
                    uart_send_char(str[i]);
                break;
            case 'x':
                ld = va_arg(args, unsigned int);
                uart_binary_to_hex(ld);
                break;

            default:
                break;
        }

    }
}
