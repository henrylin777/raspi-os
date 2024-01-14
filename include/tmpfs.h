#include "vfs.h"

#define TMPFS_TYPE_UNDEFINE     0x0
#define TMPFS_TYPE_FILE         0x1
#define TMPFS_TYPE_DIR          0x2
#define TMPFS_TYPE_DEVICE       0x3

typedef struct tmpfs_node {
    int type; /* if this node is directory then head would be list head */
    struct vnode *vfs_node;
    char data[16];
} tfs_node_t;

fs_t *tmpfs_init();
int tmpfs_mkdir(vnode_t *dir_node, vnode_t **new_node, char *name);
int tmpfs_lookup(vnode_t *dir_node, vnode_t **target, char *name);

