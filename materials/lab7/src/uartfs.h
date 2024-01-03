#include "vfs.h"

#define UARTFS_TYPE_FILE 0xab
#define UARTFS_TYPE_DIR  0xac

typedef struct uartfs_file {
    unsigned int id; /* device id */
} ufile;

typedef struct uartfs_dir {
    list_node_t head; /* list of device files */
} udir;

/* file node in cpio file system */
typedef struct uartfs_node { 
    char name[16];   /* node name */
    unsigned int id; /* device id */
    unsigned int type;
    list_node_t head;
    vnode_t *node;   /* pointer to vnode */
} unode_t;

fs_t *uartfs_init();
int uartfs_write(file_t *file, void *buf, unsigned int len);
int uartfs_read(file_t *file, void *buf, unsigned int len);
int uartfs_mknod(vnode_t *node, char *name);
