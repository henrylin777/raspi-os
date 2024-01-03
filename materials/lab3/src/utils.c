#include "utils.h"
#include "allocater.h"

int strmatch(char *a, char *b)
{
    while ((*a == *b) && *a) {
        a++;
        b++;
    }
    return ( !(*a) && !(*b)) ? 1 : 0;
}

/* convert string (hexadecimal format) to integer 
 * str   : pointer to char array
 * nchar : number of characters in str  
*/
unsigned long str2int(const char *str, int nchar) {
    unsigned long num = 0;
    for (int i = 0; i < nchar; i++) {
        num = num * 16;
        if (*str >= '0' && *str <= '9') {
            num += (*str - '0');
        } else if (*str >= 'A' && *str <= 'F') {
            num += (*str - 'A' + 10);
        } else if (*str >= 'a' && *str <= 'f') {
            num += (*str - 'a' + 10);
        }
        str++;
    }
    return num;
}
/* convert string (decimal format) to integer 
 * str   : pointer to char array
 * nchar : number of characters in str  
*/
unsigned long decstr2int(const char *str, int nchar) {
    unsigned long num = 0;
    for (int i = 0; i < nchar; i++) {
        num = num * 10;
        num += (*str - '0');
        str++;
    }
    return num;
}

/* align `data` to the multiple of `base` */
void alignto(void *data, unsigned int base) {
	unsigned long *x = (unsigned long*) data;
	unsigned long mask = base - 1;
	*x = ((*x) + mask) & (~mask);
}

/* Get the length of string without '\0' */
unsigned int strlen(char *s) {
    char *p = s;
    unsigned int i = 0;
    while (*p++)
        i++;
    return i;
}

char *strcopy(char *src) {
	unsigned int len = strlen(src) + 1;
	char *ptr = (char *) simple_malloc(len);
    if (ptr == 0)
        return 0;
    char *base = ptr;
    while (len--)
        *ptr++ = *src++;
    return base;
}
