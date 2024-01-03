#include "utils.h"
#include "uart.h"
#include "allocater.h"

/* Initialize a simple memory space for simple allocator */
#define MAX_SIZE 8192
static unsigned char SMEM_SPACE [MAX_SIZE];
static unsigned int SMEM_OFFSET = 0;


void *simple_malloc(unsigned long size) {
    if (size > MAX_SIZE || (MAX_SIZE - SMEM_OFFSET) < size)
        return (void *) 0;
    void *res = (void *) &SMEM_SPACE[SMEM_OFFSET];
    SMEM_OFFSET += size;
    return res;
} 

/* to test if simple_malloc is functioning well */
int test_malloc(char *base, char *arg) {
    int nchar = 0;
    char *p = arg;
    while(*p++)
        nchar++;

    int size = decstr2int(arg, nchar);
    for(int i = 0; i < size; i++, base++) {
        if (*base) {
            uart_send_string("Error: space is already allocated!\n");
            return 0;
        };
        *base = 'a';
    };
    *base = '\0';
    uart_send_string("test_malloc: successifully allocate ");
    uart_send_string(arg);
    uart_send_string(" bytes of memory\n");
    return 1;
}
