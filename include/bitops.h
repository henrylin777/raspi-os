/* Simple implementation of Linux bit operation
 * Reference:
 * https://elixir.bootlin.com/linux/latest/source/include/linux/bitmap.h
 * https://elixir.bootlin.com/linux/latest/source/arch/arm/include/asm/bitops.h
 */

#include "kprintf.h"

/* why unsigned long is 32 bits? */
#define BIT_PER_LONG        32
#define BYTE_PER_LONG       4
#define BIT_WORD(nr)        ((nr) >> 5)
#define BIT_SHIFT(nr)       ((nr) & 0b11111)

#define _FITST_ZERO_BIT(val)                        \
({                                                  \
    unsigned long shift, cmp;                       \
    shift = cmp = 1;                                \
    while(val & cmp) {val = val >> 1; shift++;}     \
    shift;                                          \
})     

#define FIND_FITST_BIT(val, size)           \
({                                          \
    unsigned int idx, cmp, shift = 0;       \
    cmp = 1;                                \
    for(idx = 0; idx < size, idx++) {       \
        if (val & cmp)                      \
            break;                          \
        cmp = cmp << 1; shift++;            \
    }                                       \
    shift;                                  \
})      



unsigned int div_ceil(unsigned int a, unsigned int b) {
    unsigned int ret = a / b;
    if (a % b)
        return ret + 1;
    return ret;
};

void set_bit(unsigned long *map, unsigned long nr) {
    map[BIT_WORD(nr)] |= (1UL << BIT_SHIFT(nr));
};

void clear_bit(unsigned long *map, unsigned long nr) {
    map[BIT_WORD(nr)] &= ~(1UL << BIT_SHIFT(nr));
};


uint32 test_bit(unsigned long *map, unsigned long nr) {
    return (map[BIT_WORD(nr)] & (1UL << BIT_SHIFT(nr)));
};

uint32 find_first_zero_bit(unsigned long *map, unsigned long size) {
    unsigned long idx, val, res = 0;
    for (idx = 0; idx < size; idx++) {
        val = _FITST_ZERO_BIT(map[idx]);
        if (val) {
            res = (idx * BIT_PER_LONG) + val - 1; /* for the case 0 */
            break;
        }

    }
    return res;
}   
