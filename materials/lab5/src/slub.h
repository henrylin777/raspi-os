#include "mmzone.h"

#define SLUB_BASE_ADDR 0x10020000
#define SLUB_RED_INACTIVE	0xbb
#define SLUB_RED_ACTIVE		0xcc

/* for poisoning */
#define	POISON_INUSE	0x5a	/* for use-uninitialised poisoning */
#define POISON_FREE	0x6b	/* for use-after-free poisoning */
#define	POISON_END	0xa5	/* end-byte of poisoning */

#define NUM_KMEM_CACHE 16

struct kmem_cache {
    unsigned long size;       /* the size of object in the page */
    unsigned long obj_size;   /* the actual size of object in the page */
    unsigned long offset;     /* the offset of next pointer in object */
    struct page *page;        /* the base address of page */
    struct kmem_cache *list;  /* a list of kmem_cache */
};

struct kmalloc_cache {
    struct kmem_cache *cache_list;
    struct kmem_cache cache[NUM_KMEM_CACHE];
};

unsigned long slub_alloc_object();
void *slub_malloc(unsigned long size);
void slub_init();
int slub_free(void *obj);
void slub_register_cache(void **p, unsigned long size);
void slub_info();
unsigned long get_order(unsigned long size);
