#include "slub.h"
#include "buddy.h"
#include "utils.h"
#include "uart.h"

#define slub_min_objects 64
#define slub_max_order 10

struct kmalloc_cache *slub_allocator = 0;
unsigned long cache_idx = 0;
/* minimum numbers of objects should be considered when allocating a cache */


void mem_write(char *p, unsigned long size, char val) {
    for (; size; size--, p++)
        *p = val;
}

inline unsigned long get_order(unsigned long size) {
    unsigned long num = PAGE_SIZE;
    int cnt = 0;
    while (size > num) {
        num = (num << 1);
        cnt++;
    };
    return cnt;
}

/* find out the maximum order under the fraction */
unsigned long calculate_order(unsigned long obj_size, unsigned long num_objs, unsigned long fraction) {
    unsigned long rem, order, total_size, size;
    size = (obj_size * num_objs) + sizeof(struct page);
    order = get_order(size);
    while (order < slub_max_order) {
        total_size = (PAGE_SIZE << order);
        rem = total_size % obj_size;
        if (!rem) break;
        if (total_size / rem > fraction)
            break;
        order++;
    }
    return order;
}

/* Attempt to find out the best configuration in a slab. First we increase the acceptable waste, 
 * then reduce the minimum required objects in a slab.
 * reference: https://elixir.bootlin.com/linux/latest/source/mm/slub.c#L4137
 */
unsigned long slub_calculate_sizes(unsigned long obj_size, unsigned long *objects) {
    unsigned long fraction;
    unsigned long num_objs = slub_min_objects;
    
    unsigned long order = 0;
    while (num_objs > 1) {
        fraction = 16;
        while (fraction >= 4) {
            order = calculate_order(obj_size, num_objs, fraction);
            if (order < slub_max_order) {
                *objects = num_objs;
                return order;
            }
            fraction /= 2;
        }
        num_objs--;
    }
    /* try to allocate a single object in this slab */
    order = calculate_order(obj_size, 1, 16);
    if (order < slub_max_order) {
        *objects = 1;
        return order;
    }
    uart_send_string("cannot allocate a slub using slub_max_order");
    while(1){};
}

void slub_init_object(struct kmem_cache *cache, struct page *page, unsigned long objects) {
    
    /* object layout
     * +----------+-------------------+----------+-------+--------------+-------+
     * | red zone |  allocable space  | red zone | align | next pointer | align |     
     * +----------+-------------------+----------+-------+--------------+-------+
     */

    unsigned long end = alignto(page->mapping, (1 << 12));
    unsigned long start = page->mapping;
    DEBUG("slub allocator: init list of objects from address: ", (unsigned long) start, 0);
    unsigned long total_size = end - start;
    mem_write((char *)start, total_size, POISON_INUSE);

    /* setup next pointer */
    unsigned long next_addr = start + cache->obj_size;
    unsigned long *next_ptr = (unsigned long *)(start + cache->offset);
    // DEBUG("[*] start: ", start, 0);
    // DEBUG("[*] next_ptr: ", (unsigned long)next_ptr, 0);
    // DEBUG("[*] next_addr: ", next_addr, 0);
    for (unsigned long i = 0; i < objects; i++) {
        // DEBUG("[*] next_ptr: ", (unsigned long)next_ptr, 0);
        *next_ptr = next_addr;
        next_ptr = (unsigned long *)(next_addr + cache->offset);
        next_addr += cache->obj_size;
    }
}


unsigned long slub_alloc_object(struct kmem_cache *cache, struct page *page) {
    unsigned long object = page->mapping;
    // unsigned long object = (unsigned long) page + sizeof(struct page);
    mem_write((char *)object, cache->offset, SLUB_RED_ACTIVE);
    mem_write((char *)(object + 8), cache->size, POISON_FREE); /* offset red zone */
    /* point to the next available object */
    page->mapping = object + cache->offset;
    page->inuse++;
    return object + 8;
}   


void slub_init_cache(struct kmem_cache *cache, unsigned long size) {
    
    cache->size = size;
    unsigned long offset = 16 + cache->size;
    /* round up object size so that the free pointer could be placed at word boundary */
    cache->offset = alignto(offset, sizeof(void *));
    cache->obj_size = alignto(cache->offset + 8, sizeof(void *));
    unsigned long num_objs;
    unsigned long order = slub_calculate_sizes(cache->obj_size, &num_objs);
    struct page *page = buddy_alloc_page(order);
    uart_send_string("slub allocator: got a page at ");
    uart_binary_to_hex((unsigned long)page);
    uart_send_string(" for initializing a kmem_cache\r\n");
    cache->page = page;
    cache->page->flag = ps_slab;
    cache->page->slub_cache = cache;
    cache->page->order = order;
    cache->page->objects = num_objs;
    cache->page->inuse = 0;
    slub_init_object(cache, page, cache->page->objects);
    
    cache->list = slub_allocator->cache_list;
    slub_allocator->cache_list = cache;
} 

void slub_free_object(struct kmem_cache *cache, struct page *page, unsigned long addr) {
    unsigned long *ptr = (unsigned long *)(addr - 8 + cache->offset);
    *ptr = page->mapping;
    page->mapping = addr;

    mem_write((char *)addr + cache->size, sizeof(void *), SLUB_RED_INACTIVE);
    mem_write((char *)addr - 8, sizeof(void *), SLUB_RED_INACTIVE);
    mem_write((char *)addr, cache->size, POISON_FREE);
    page->inuse--;
}

int slub_free(void *obj) {
    unsigned long addr = ((unsigned long) obj) & 0xFFFFF000;
    struct page *page = (struct page *)buddy_get_page_addr(addr);
    if (page->flag != ps_slab) {
        return 0;
    }
    struct kmem_cache *cache = page->slub_cache;
    slub_free_object(cache, page, (unsigned long) obj);
#ifdef CONFIG_MM_DEBUG
    uart_send_string("pointer to address ");
    uart_binary_to_hex((unsigned long) obj);
    uart_send_string(" successifully free out\n");
#endif
    return 1;
}

void slub_register_cache(void **p, unsigned long size) {
    if (cache_idx >= NUM_KMEM_CACHE) {
        uart_send_string("not available space for allocating new kmem_cache");
        return;
    };
    struct kmem_cache *cache = &slub_allocator->cache[cache_idx];
    cache_idx++;
    slub_init_cache(cache, size);
    *p = cache;
}

void slub_init() {
    struct kmem_cache *cache;
    slub_allocator = (struct kmalloc_cache *) SLUB_BASE_ADDR;
    slub_allocator->cache_list = 0;

    cache = &slub_allocator->cache[cache_idx++];
    slub_init_cache(cache, 8);

    cache = &slub_allocator->cache[cache_idx++];
    slub_init_cache(cache, 16);

    uart_send_string("slub allocator initialization complete\n");
}

void slub_info() {
    struct kmem_cache *cache;
    uart_send_string("there are ");
    uart_send_int(cache_idx);
    uart_send_string(" kmem_cache in the slub allocator\n");
    for (unsigned long idx = 0; idx < cache_idx; idx++) {
        cache = &slub_allocator->cache[idx];
        uart_send_int(cache->page->inuse);
        uart_send_string(" used objects in kmem_cache of size ");
        uart_send_int(cache->size);
        DEBUG(", total objects ", cache->page->objects, 1);
    }
} 


void *slub_malloc(unsigned long size) {
    unsigned long ptr;
    struct kmem_cache *cache;
    cache = slub_allocator->cache_list;
    while(cache) {
        if (size <= cache->size)
            break;
        cache = cache->list;
    }
    if (!cache) {
        uart_send_string("slub allocator: cannot malloc for size ");
        uart_send_int(size);
        uart_send_string(", initialize a new kmem_cache\r\n");
        cache = &slub_allocator->cache[cache_idx++];
        slub_init_cache(cache, size);
    }

    ptr = slub_alloc_object(cache, cache->page);
    // DEBUG("slub allocator: allocate object at ", ptr, 0);
    return (void *)ptr;
}
