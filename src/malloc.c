#include <buddy.h>
#include <slub.h>
#include <mmzone.h>
#include <utils.h>
#include <uart.h>
#include <list.h>
#include <timer.h>

struct kmem_cache *IRQ_ALLOC;    /* irq node allcaotor */
struct kmem_cache *LNODE_ALLOC;  /* list_node_t allocator */

void mem_init() {
    buddy_init();
    slub_init();
    slub_register_cache((void **)&LNODE_ALLOC, sizeof(list_node_t));
}

// void *mem_alloc(unsigned long size) {
//     void *ptr = slub_malloc(size);
//     return ptr;
// }

// void mem_free(void *p) {
//     return;
// }

void mem_print(unsigned long addr) {
    unsigned long val;
    unsigned int n, flag;

    for (unsigned long i = 0; i < 45; i++) {
        val = *(unsigned long *)addr;
        n = flag = 0;
        uart_binary_to_hex(addr);
        uart_send_string(": ");
        for (int c = 28; c >= 0; c -= 4) {
            n = (val >> c)& 0xF;  /* get highset 4 bits */
            n += n > 9 ? 0x37 : 0x30;
            flag ^= 1;    
            uart_send_char(n);
            if (!flag) uart_send_char(' ');
        }
        // uart_binary_to_hex(val);
        uart_send_string("\n");
        addr += 8;
    }
    uart_send_string("\n");
}



void mem_show_page(unsigned long start, unsigned long end) {
    unsigned long flag;
    struct page *page;
    uart_send_string("frame array:\n");
    for (unsigned long addr = start; addr < end; addr += PAGE_SIZE) {
        uart_binary_to_hex(addr);
        page = (struct page*) addr;
        flag = page->flag;
        uart_send_string(": ");
        uart_print_addr(flag);
        if (flag == ps_reserved) {
            uart_send_string(" (reserved)\n");
        } else if (flag == ps_used) {
            uart_send_string(" (used)\n");
        } else if (flag == ps_buddy) {
            uart_send_string(" (buddy)\n");
        } else {
            uart_send_string(" (available)\n");
        }
    }
}

/* allocate a memory space with size and return a pointer */
void *kmalloc(unsigned long size) {
    if (!size) {
        uart_send_string("empty size\n");
        return (void *) 0;
    };
    void *ptr;

    if (size < PAGE_SIZE) {
        ptr = slub_malloc(size);
    } else {
        unsigned long order = get_order(size);
        ptr = buddy_alloc_page(order);
        uart_send_string("allocate a page at ");
        uart_binary_to_hex((unsigned long) ptr);
        uart_send_string(" for size ");
        uart_send_int(size);
        uart_send_string("\n");
    }
        
    return ptr;
}

void kfree(void *ptr) {
    if (slub_free(ptr) != 0) {
        return;
    }
#ifdef CONFIG_MM_DEBUG
    uart_binary_to_hex((unsigned long) ptr);
    uart_send_string(" is not managed by slub allocator, use buddy_free\n");
#endif
    buddy_free((unsigned long)ptr);
}

void *mymalloc(unsigned long size) {
    return kmalloc(size);
}
