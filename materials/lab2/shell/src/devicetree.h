#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009

/* pointer to the base address of file */
extern void *BASE_DTB;

/*TODO: use typedef to alias unsigned int instead of hard code */

/* Header of devicetree */
struct dtb_header {
    unsigned int magic;             // contain the value 0xd00dfeed (big-endian).
    unsigned int totalsize;         // the total size of devicetree in byte
    unsigned int off_dt_struct;     // the size of offset of the structure block in bytes from the beginning of the header
    unsigned int off_dt_strings;    // the size of offset of the strings block in bytes from the beginning of the header 
    unsigned int off_mem_rsvmap;    // the size of offset of the memory reservation block in bytes from the beginning of the header
    unsigned int version;           // the version of devicetree data structure
    unsigned int last_comp_version; // the lowest version of device tree with which the version used is backward compatible. 
    unsigned int boot_cpuid_phys;   // the physical ID of system's boot CPU 
    unsigned int size_dt_strings;   // the length in bytes of the strings block section
    unsigned int size_dt_struct;    // the length in bytes of the structure block section of the device tree blob 
};

/* Memory researvation block */
struct dtb_reserve_entry {
    unsigned long long address;
    unsigned long long size;
};


typedef void (*print_callback) (char *, char *);
extern print_callback funcptr;

void parse_node(char *pnode, char *strblock);
int traverse_dtb(char *device);
void parse_prop(char *nprop, char *strblock, unsigned int len, unsigned int nameoff);
