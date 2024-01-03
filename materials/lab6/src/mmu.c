#include "mmu.h"
#include "utils.h"
#include "uart.h"
#include "malloc.h"
#include "buddy.h"

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB          ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT      (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

/*
 * mair register attributes:
 *
 *   n = AttrIndx[2:0]
 *            n    MAIR
 *   DEVICE_nGnRnE    000    00000000
 *   NORMAL_NC        001    01000100
 */
#define MAIR_DEVICE_nGnRnE  0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE  0
#define MAIR_IDX_NORMAL_NOCACHE 1

#define PD_TABLE    0b11
#define PD_BLOCK    0b01
#define PD_ACCESS       (1 << 10)
#define PD_PXN          ((unsigned long)1 << 53)
#define PD_NSTABLE      ((unsigned long)1 << 63)
#define PD_UXNTABLE     ((unsigned long)1 << 60)
#define PD_MAIR_DEVICE_IDX (MAIR_IDX_DEVICE_nGnRnE << 2)
#define PD_MAIR_NORMAL_IDX (MAIR_IDX_NORMAL_NOCACHE << 2)

// Block Entry
#define PD_BE PD_ACCESS | PD_BLOCK
// Level 3 Block Entry
#define PD_L3BE PD_ACCESS | PD_TABLE

#define BOOT_PGD ((pt_t *)0x1000)
#define BOOT_PUD ((pt_t *)0x2000)
#define BOOT_PMD ((pt_t *)0x3000)


#define PAGE_SHIFT	 		12
#define TABLE_SHIFT 	    9
#define PGD_SHIFT			PAGE_SHIFT + 3*TABLE_SHIFT
#define PUD_SHIFT			PAGE_SHIFT + 2*TABLE_SHIFT
#define PMD_SHIFT			PAGE_SHIFT + TABLE_SHIFT

#define UNUSED(x) (void)(x)

/* create a PGD table for a process */
// void pt_create(unsigned long task) {
//     pt_t prev_pt, pt;
//     prev_pt = (pt_t) kmalloc(sizeof(PAGE_TABLE_SIZE));
//     memzero(prev_pt, PAGE_TABLE_SIZE);
//     /* map the user space */
//     for (int lv = 0; lv < 3; lv++) {
//         pt_t pt = (pt_t) kmalloc(sizeof(PAGE_TABLE_SIZE));
//         memzero(pt, PAGE_TABLE_SIZE);
//         prev_pt[0] = pt;
//         prev_pt = pt;
//     }
// }

unsigned long alloc_kernel_page() {
    pt_t page = (pt_t) buddy_alloc_page(0);
    return VA_START + page;
}

pt_t map_table(pt_t *table, unsigned long shift, unsigned long va, int *new_table){

    uart_send_string("in map_table\r\n");
    DEBUG("table: ", (unsigned long) table, 0);
    while(1){};
    // unsigned long idx = (va >> shift);

}


void map_page(task_struct *task, pt_t va, pt_t page) {

    int new_table;
    pt_t pgd = (pt_t) buddy_alloc_page(0);
    map_table((pt_t *)(pgd + VA_START), PGD_SHIFT, va, &new_table);
    return;
}

pt_t alloc_user_page(task_struct *current, unsigned long va) {
    
    pt_t page = (pt_t) buddy_alloc_page(0);
    map_page(current, va, page);
    return page + VA_START;
}


void mmu_init(void)
{
    unsigned int sctlr_el1;

    /* Set TCR_EL1 register */
    write_sysreg(TCR_EL1, TCR_CONFIG_DEFAULT);

    /* set mair register */
    write_sysreg(MAIR_EL1,
                (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8)) |
                (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8)));
    
    // Set Identity Paging
    // 0x00000000 ~ 0x3f000000: Normal
    // 0x3f000000 ~ 0x40000000: Device
    // 0x40000000 ~ 0x80000000: Device
    BOOT_PGD[0] = (unsigned long)BOOT_PUD | PD_NSTABLE | PD_UXNTABLE | PD_TABLE;

    BOOT_PUD[0] = (unsigned long)BOOT_PMD | PD_TABLE;
    BOOT_PUD[1] = 0x40000000 | PD_MAIR_DEVICE_IDX | PD_BE;

    for (int i = 0; i < 504; ++i) {
        BOOT_PMD[i] = (i * (1 << 21)) | PD_MAIR_NORMAL_IDX | PD_BE;
    }

    for (int i = 504; i < 512; ++i) {
        BOOT_PMD[i] = (i * (1 << 21)) | PD_MAIR_DEVICE_IDX | PD_BE;
    }

    write_sysreg(TTBR0_EL1, BOOT_PGD);
    write_sysreg(TTBR1_EL1, BOOT_PGD);

    // Enable MMU
    sctlr_el1 = read_sysreg(SCTLR_EL1);
    write_sysreg(SCTLR_EL1, sctlr_el1 | 1);
}
