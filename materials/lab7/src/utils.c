#include "utils.h"

int strmatch(char *a, char *b)
{
    while ((*a == *b) && *a) {
        a++;
        b++;
    }
    return ( !(*a) && !(*b)) ? 1 : 0;
}

/* convert string (hexadecimal format) to integer */
unsigned long hexstr_to_int(char *str) {
    int nchar = strlen(str);
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
/* convert string (decimal format) to integer */
unsigned long decstr_to_int(char *str) {
    int nchar = strlen(str);
    unsigned long num = 0;
    for (int i = 0; i < nchar; i++) {
        num = num * 10;
        num += (*str - '0');
        str++;
    }
    return num;
}

/* align `data` to the multiple of `base` */
unsigned long alignto(unsigned long data, unsigned int base) {
        unsigned long x;
        unsigned long mask = base - 1;
        x = (data + mask) & (~mask);
    return x;
}

/* Get the length of string without '\0' */
unsigned int strlen(char *s) {
    char *p = s;
    unsigned int i = 0;
    while (*p++)
        i++;
    return i;
}

/* convert integer to string format */
void int2string(int i, char *p){
    char const digit[] = "0123456789";
    if (i < 0) {
        *p++ = '-';
        i *= -1;
    }
    int shifter = i;
    do { //Move to where representation ends
        ++p;
        shifter = shifter / 10;
    } while(shifter);
    *p = '\0';
    do { //Move back, inserting digits as u go
        *--p = digit[i % 10];
        i = i / 10;
    } while(i);
}

int strncmp(char *a, char *b, int n) {
    while (n > 0) {
        if (*a != *b)
            return 0;
        a++; b++;
        n--;
    };
    return 1;
}

int strcopy(char *dst, char *src) {
    int len = 0;
    for (; *src; src++, dst++, len++) {
        *dst = *src;
    };
    *dst = '\0';
    return len;
}

void strncopy(char *dst, char *src, int n) {
    while(n--) {
        *dst = *src;
        dst++; src++;
    };
    *dst = '\0';
}
