#pragma once

#include "list.h"

#define BASE_ADDR 0x00000000
#define END_ADDR 0x3C000000
#define PAGE_SIZE 0x1000 /* 4096 bytes (4kb) */
#define NUM_PAGE (END_ADDR - BASE_ADDR) / PAGE_SIZE
#define MAX_ORDER 10


/* page or slab */
typedef struct page {
    unsigned long idx;          /* the index of page frame */
    unsigned long flag;         /* the status of page frame */
    unsigned long mapping;      /* the start address of allocable space in the page */ 
    unsigned long inuse;         /* number of objects in use */
    unsigned long objects;       /* total number of objects in this page */
    unsigned long order;         
    union {
        struct kmem_cache *slub_cache;
        struct free_area *free_area;
    };
    list_node_t node;           /* pointer to the next page */
} page_node_t;


struct free_area {
    list_node_t free_list;   /* a list of free pages */
    unsigned long n_free;    /* number of free pages */
};

struct zone {
    struct free_area free_area[MAX_ORDER + 1]; /* free areas of different orders */
    unsigned int pagearray[NUM_PAGE];
    unsigned long page_size;
};
