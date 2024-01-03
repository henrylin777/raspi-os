#include "sche.h"

#define PAGE_TABLE_SIZE 0x1000

#define PT_R    0x0001
#define PT_W    0x0002
#define PT_X    0x0004

#define VMA_R       PT_R
#define VMA_W       PT_W
#define VMA_X       PT_X
// PA
#define VMA_PA      0x0008
// Kernel VA
#define VMA_KVA     0x0010
// Anonymous
#define VMA_ANON    0x0020

#define VA_START 0xffff000000000000

typedef unsigned long pt_t;

void mmu_init();
pt_t alloc_user_page(task_struct *task, unsigned long va);
