

#define BASE_BUDDY_ADDR 0x10000000

/* page state */
#define ps_buddy 0x21212121
#define ps_init  0x81818181
#define ps_slab  0x31313131
#define ps_used  0xA1A1A1A1
#define ps_reserved 0xFFFFFFFF
#define ps_available 0



void buddy_init();
void *buddy_alloc_page(unsigned long order);
void bfree(void *ptr);
void *find_buddy(char *ptr, unsigned int size);
void bmerge(char *ptr, unsigned int size);
void buddy_show();
void buddy_info();
unsigned long buddy_get_page_addr(unsigned long addr);
int buddy_free(unsigned long ptr);
