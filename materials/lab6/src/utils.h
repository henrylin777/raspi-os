#pragma once 

/* reference: https://elixir.bootlin.com/linux/latest/source/tools/lib/perf/mmap.c#L299 */
#define read_sysreg(r) __extension__({                       \
    unsigned long __val;                               \
    asm volatile("mrs %0, " #r : "=r" (__val)); \
    __val;                                      \
})

/* Reference from https://elixir.bootlin.com/linux/latest/source/arch/arm64/include/asm/sysreg.h#L1281 */
#define write_sysreg(r, v) do {    \
    unsigned long __val = (unsigned long)(v);          \
    asm volatile("msr " #r ", %x0" \
             : : "rZ" (__val));    \
} while (0)


int strmatch(char *a, char *b);
unsigned long hexstr_to_int(char *str);
unsigned long decstr_to_int(char *str);
unsigned long alignto(unsigned long data, unsigned int base);
unsigned int strlen(char *p);
char *strcopy(char *src);
void int2string(int data, char *string);

/* helper function in utils assembly code */
void memcopy(char *dst, char *src, unsigned long size);
void memzero(unsigned long addr, unsigned long size);
void delay(unsigned int sec);
unsigned int get32(unsigned int reg);
void put32(unsigned int reg, unsigned int val);
