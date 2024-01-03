#include "utils.h"
#include "uart.h"
#include "buddy.h"
#include "allocater.h"
#include "list.h"
#include "cpio.h"
#include "mmzone.h"

struct zone *buddy_allocator = 0;

/* Get the physical address of page frame with idx */
#define get_addr_from_idx(idx) (BASE_ADDR + idx * PAGE_SIZE)

#define buddy_mark_used(addr)                        \
    ((struct page *)addr)->flag = ps_used;           \
    buddy_allocator->pagearray[(addr >> 12)] = addr; \

#define buddy_mark_buddy(idx, addr, order)                                      \
    struct page *ptr = (struct page *)(addr + PAGE_SIZE);                       \
    for (unsigned long i = idx; i < pow_of_two(order); i++, ptr += PAGE_SIZE) { \
        ptr->flag = ps_buddy;                                                   \
        buddy_allocator->pagearray[i] = ps_buddy;                               \
    }
        

/* buddy_mark_available: mark page as available
 * @addr: the base address of page frame 
 */
#define buddy_mark_available(addr)                           \
    ((struct page *)addr)->flag = ps_available;              \
    buddy_allocator->pagearray[(addr >> 12)] = ps_available;


/* get the index of buddy page by order */
#define get_buddy_idx(idx, order) idx ^ (1 << order)

/* get the base address of buddy page by order */
unsigned long get_buddy_addr(unsigned long idx, unsigned long order) {
    unsigned long buddy_idx = get_buddy_idx(idx, order);
    return (buddy_idx << 12);
}




unsigned long pow_of_two(unsigned long order) {
    if (order == 0) return 1;
    unsigned long res = 1;
    while (order--)
        res *= 2;
    return res;
}

void ERROR(char *msg) {
    uart_send_string("\nERROR: ");
    uart_send_string(msg);
    while(1){};
}


void buddy_show(struct zone *buddy_allocator, unsigned long idx, int offset) {
    unsigned int val;
    unsigned long page;
    for (int i = 0; i < offset; i++) {
        page = (idx << 12);
        uart_binary_to_hex(page);;
        uart_send_string(":\t");
        val = buddy_allocator->pagearray[idx];
        uart_binary_to_hex(val);
        uart_send_string("\n");
        idx++;
    }
}

void buddy_info() {
    for (int i = 0; i <= MAX_ORDER; i++) {
        uart_send_string("there are ");
        uart_send_int(buddy_allocator->free_area[i].n_free);
        DEBUG(" blocks available of order ", i, 1);
    };

}

void buddy_reserve_memory(struct zone *buddy_allocator, unsigned long start, unsigned long end) {
    unsigned int start_idx = (start >> 12);
    end = alignto(end, (1 << 12));
    unsigned int end_idx = (end >> 12);
    for (unsigned int i = start_idx; i < end_idx; i++) {
        buddy_allocator->pagearray[i] = ps_reserved;
    };
}

/* calculate the maximum order and available pages in this space */
unsigned long buddy_cal_free_pages(struct zone *buddy_allocator, unsigned int idx, unsigned long *free_pages) {
    unsigned int size = 0;
    while(buddy_allocator->pagearray[idx] != ps_reserved && idx < NUM_PAGE) {
        size++; idx++;
    }
    // DEBUG("got availabe pages ", size, 1);
    *free_pages = size;
    size = size | (size >> 1);
    size = size | (size >> 2);
    size = size | (size >> 4);
    size = size | (size >> 8);
    size = size | (size >> 16);
    size = size - (size >> 1);
    unsigned long order = 0;
    while (size) {
        size = (size >> 1); order++;
    }
    return order - 1;
}

void buddy_init_page(struct free_area *area, unsigned long idx, unsigned long order) {
    struct page *page = (struct page *)get_addr_from_idx(idx);
    page->idx = idx;
    page->flag = ps_init;
    page->free_area = area;
    page->order = order;
    page->mapping = (unsigned long)page + sizeof(struct page);
}


/* 32 4 4 */
void buddy_init() {
    list_node_t *head;
    struct free_area *area;
    struct page *page;
    /* initialize zone by startup allocator */
    buddy_allocator = (struct zone *) BASE_BUDDY_ADDR;
    buddy_allocator->page_size = PAGE_SIZE;

    buddy_reserve_memory(buddy_allocator, 0, 0x1000);               /* spin table for multicore boot */
    buddy_reserve_memory(buddy_allocator, 0x1000, 0x80000);         /* flatten device tree */
    buddy_reserve_memory(buddy_allocator, 0x00080000, 0x000B0000);  /* kernel image in the physical memory */
    buddy_reserve_memory(buddy_allocator, 0x20000000, 0x20030000);  /* initramfs */
    buddy_reserve_memory(buddy_allocator, 0x10000000, 0x10010000);  /* for buddy system */
    buddy_reserve_memory(buddy_allocator, 0x10010000, 0x10020000);  /* slub allocator */
    
    /* initialize free list of each order */
    for (int order = 0; order <= MAX_ORDER; order++) {
        area = &buddy_allocator->free_area[order];
        head = &area->free_list;
        list_init(head);
    };

    /* initialize memory block */
    unsigned long max_order, idx, num_pages, free_pages;
    idx = 0;
    while(idx < NUM_PAGE) {
        if (buddy_allocator->pagearray[idx] == ps_reserved) {
#ifdef CONFIG_MM_DEBUG
            uart_send_string("page with address ");
            uart_binary_to_hex(idx << 12);
            uart_send_string(" is reserved, jump to next available page\n");
#endif
            while (buddy_allocator->pagearray[idx] == ps_reserved)
                idx++;
        };
        free_pages = 0;
        max_order = buddy_cal_free_pages(buddy_allocator, idx, &free_pages);
        if (max_order > MAX_ORDER) max_order = MAX_ORDER;

        while(free_pages) {
            num_pages = pow_of_two(max_order);
            area = &buddy_allocator->free_area[max_order];
            head = &area->free_list;
            for (unsigned long i = idx; i < idx + num_pages; i++) {
                buddy_init_page(area, i, max_order);
            };
            page = (struct page *)get_addr_from_idx(idx);
            list_add_tail(&page->node, head);
            area->n_free++;
            buddy_mark_buddy(idx, get_addr_from_idx(idx), max_order);
#ifdef CONFIG_MM_DEBUG
            uart_send_string("build a memory block at ");
            uart_binary_to_hex(get_addr_from_idx(idx));
            DEBUG(" with order ", max_order, 1);
#endif
            idx += num_pages;
            free_pages -= num_pages;
            if (free_pages < num_pages)
                max_order -= 2;
            if (max_order <= 2)
                max_order = 0;
        }
        // while(1){};
    }
    uart_send_string("buddy system initialization complete\n");
    buddy_info();
}

/* insert a page into free_list */
inline void buddy_insert_page(struct free_area *area, struct page *page, unsigned long idx, unsigned long order, unsigned long mapping) {
    list_add_tail(&page->node, &area->free_list);
    page->flag = ps_available;
    page->idx = idx;
    page->mapping = mapping;
    page->free_area = area;
    page->order = order;
    area->n_free++;
}

/* unlink this page */
void buddy_del_page(struct free_area *area, list_node_t *head, page_node_t *page) {
    if (list_empty(head))
        return;
    list_remove(&page->node);
    area->n_free--;
}

int page_is_buddy(struct page *page, struct page *buddy) {
    unsigned long flag = buddy->flag; 
    if(flag == ps_used || flag || ps_reserved)
        return 0;
    if (page->free_area != buddy->free_area)
        return 0;
    return 1;
}
/* find out the buddy page and return the address */
page_node_t *get_buddy_page(struct page *page, unsigned long idx, unsigned long order, 
                            unsigned long *buddy_idx)
{
    struct page *buddy = (struct page *) get_buddy_addr(idx, order);
    if (page_is_buddy(page, buddy)) {
        *buddy_idx = idx + (buddy->idx - idx);
        return buddy;
    }
    return 0;
}

/* this function ask for free page from higher order if needed
 * if yes, then we need to create page from high to low
 */
void *buddy_expand(struct page *page, unsigned long idx, unsigned long order, 
                   unsigned long cur_order)
{
    unsigned long buddy_idx;
    struct page *buddy_page;
    struct free_area *area;
    while (order < cur_order) {
        cur_order--;
        area = &buddy_allocator->free_area[cur_order];
        buddy_page = (struct page *)get_buddy_addr(idx, cur_order);
        buddy_idx = idx + pow_of_two(cur_order);
#ifdef CONFIG_MM_DEBUG
        uart_send_string("insert page at ");
        uart_binary_to_hex((unsigned long) buddy_page);
        DEBUG(" into order ", cur_order, 1);
#endif
        buddy_insert_page(area, buddy_page, buddy_idx, cur_order, buddy_page->mapping);
        buddy_mark_available((unsigned long) buddy_page);
    };
    /* setup page information */
    buddy_mark_used((unsigned long) page);
#ifdef CONFIG_MM_DEBUG
    uart_send_string("buddy system: allocate a memory block of order ");
    uart_send_int(order);
    DEBUG(" at ", (unsigned long) page, 0);
#endif
    return (void *)page;
}

/* alloc one available page and return the pointer of base page address */
void *buddy_alloc_page(unsigned long order)
{
    list_node_t *head;
    struct page *page;
    struct free_area *area;

    for (unsigned long cur_order = order; cur_order <= MAX_ORDER; ++cur_order) {
        area = &buddy_allocator->free_area[cur_order];
        head = &area->free_list;
        if (area->n_free == 0) {
#ifdef CONFIG_MM_DEBUG
            uart_send_string("order ");
            uart_send_int(cur_order);
            uart_send_string(" no pages available, go to higher order\n");
#endif
            continue;
        }
        page = list_entry(head->next, page_node_t, node);
#ifdef CONFIG_MM_DEBUG
        uart_send_string("got available page of order ");
        uart_send_int(cur_order);
        DEBUG(" at ", (unsigned long) page, 0);
        if (cur_order != order) {
            uart_send_string("delete this page and insert new pages into lower order\n");
        } else {
            uart_send_string("delete this page for allocating\n");
        }
#endif
        buddy_del_page(area, head, page);
        buddy_mark_used((unsigned long)page);
        return buddy_expand(page, page->idx, order, cur_order);
    }
    DEBUG("Cannot allocate memory block for order ", order, 1);
    return 0;
}

/* given the base of page address and return the base address of page if it is buddy */
unsigned long buddy_get_page_addr(unsigned long addr) {
    unsigned long idx = (addr >> 12);
    return buddy_allocator->pagearray[idx];
}

int buddy_free_page(struct page *page, unsigned long idx, unsigned long order)
{
    struct free_area *area = &buddy_allocator->free_area[order];
    list_node_t *head = 0;
    page_node_t *buddy;
    unsigned long buddy_idx = 0;
    unsigned long cur_order = 0;
    
    /* check if we need to merge from current order to higher order */
    while (cur_order < MAX_ORDER) {
        buddy = get_buddy_page(page, idx, cur_order, &buddy_idx);
        if (!buddy)
            break;
#ifdef CONFIG_MM_DEBUG
        uart_send_string("merge buddy page of order ");
        uart_send_int(order);
        uart_send_string("\n");
#endif
        area = &buddy_allocator->free_area[cur_order];
        head = &area->free_list;
        buddy_del_page(area, head, buddy);
        idx = buddy_idx & idx;
        cur_order++;
    };
    if (cur_order != order && cur_order) {
        order = cur_order - 1;
        area = &buddy_allocator->free_area[cur_order];
    }
#ifdef CONFIG_MM_DEBUG
    uart_send_string("return and insert a page into order ");
    uart_send_int(order);
    uart_send_string("\n");
#endif
    buddy_insert_page(area, page, idx, order, page->mapping);
    buddy_mark_available((unsigned long) page);
    buddy_mark_buddy(idx, page, order);
#ifdef CONFIG_MM_DEBUG
        uart_send_string("pointer to address ");
        uart_binary_to_hex((unsigned long) page);
        uart_send_string(" successufully free out\n");
#endif
    return 1;
}

int buddy_free(unsigned long ptr) {
    struct page *page = (struct page *)(ptr  & 0xFFFFF000);
    unsigned long idx = (ptr >> 12);
    unsigned long order = page->order;
    buddy_free_page(page, idx, order);
    return 1;
}
