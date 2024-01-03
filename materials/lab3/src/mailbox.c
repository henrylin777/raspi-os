#include "gpio.h"
#include "mailbox.h"

/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) MAILBOX[36];

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000


/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mailbox_call()
{
    unsigned int r = (((unsigned int)((unsigned long) &MAILBOX) & ~0xF) | (MBOX_CH_PROP&0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if(r == *MBOX_READ)
            /* is it a valid successful response? */
            return MAILBOX[1]==MBOX_RESPONSE;
    }
    return 0;
}

void get_board_revision(){
    /* buffer size in byte */
    MAILBOX[0] = 7 * 4;
    MAILBOX[1] = MBOX_REQUEST;
        
    /* tags begin */
    MAILBOX[2] = MBOX_TAG_GETBOARD;   /* tag identifier */
    MAILBOX[3] = 4;                   /* request/response buffer size in bytes */
    MAILBOX[4] = 0;                   /* request/response code */
    MAILBOX[5] = 0;                   /* clear output buffer */
    /* tags end */ 

    MAILBOX[6] = MBOX_TAG_LAST;
    mailbox_call();
}

void get_arm_mem(){
    MAILBOX[0] = 8 * 4;
    MAILBOX[1] = MBOX_REQUEST;

    /* tags begin */
    MAILBOX[2] = MBOX_TAG_GETARMMEM; /* tag identifier */
    MAILBOX[3] = 8;                  /* request/response buffer size in bytes */
    MAILBOX[4] = 0;                  /* request/response code */
    MAILBOX[5] = 0;                  /* clear output buffer */
    MAILBOX[6] = 0;                  /* clear output buffer */
    /* tags end */ 

    MAILBOX[7] = MBOX_TAG_LAST;
    mailbox_call();
}



void get_serial_number()
{
    MAILBOX[0] = 8 * 4;                 
    MAILBOX[1] = MBOX_REQUEST;         
    MAILBOX[2] = MBOX_TAG_GETSERIAL;   
    MAILBOX[3] = 8;                    
    MAILBOX[4] = 8;
    MAILBOX[5] = 0;
    MAILBOX[6] = 0;
    MAILBOX[7] = MBOX_TAG_LAST;
    mailbox_call();
}
