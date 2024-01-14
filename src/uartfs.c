#include <uart.h>
#include <uartfs.h>
#include <vfs.h>
#include <utils.h>
#include <malloc.h>
#include <list.h>

int uartfs_mount(fs_t *fs, struct mount *mount);
int uartfs_lookup(vnode_t *dir_node, vnode_t **target, char *name);
int uartfs_mknode(vnode_t *dir_node, vnode_t **target, char *name);
int uartfs_mkdir(vnode_t *dir_node, vnode_t **target, char *dirname);
int uartfs_getname(vnode_t *node, char **name);
int uartfs_read(file_t *file, void *buf, unsigned int len);
int uartfs_mknod(vnode_t *node, char *name);

static fs_t UARTFS = {
    .name = "uartfs",
    .mount = uartfs_mount,
};

static vnode_ops_t UARTFS_VNODE_OPS = {
    .lookup = uartfs_lookup,
    .create = uartfs_mknode,
    .mkdir = uartfs_mkdir,
};

static file_ops_t UARTFS_FILE_OPS = {
    .read = uartfs_read,
    .write = uartfs_write,
};


vnode_t *UARTFS_ROOT;

int uartfs_getname(vnode_t *node, char **name){
    unode_t *n = node->internal;
    *name = n->name;
    return 1;
}


int uartfs_mount(fs_t *fs, struct mount *mount) {
    uart_send_string("do uartfs_mount\r\n");
    vnode_t *node = mount->root;

    node->v_ops = UARTFS_ROOT->v_ops;
    node->f_ops = UARTFS_ROOT->f_ops;
    node->internal = UARTFS_ROOT->internal;
    node->mount = mount;
    
    /* TODO: free node */
    return 1;
};

/* find the file with name under dir_node and set target to it */
int uartfs_lookup(vnode_t *dir_node, vnode_t **target, char *name) {
    unode_t *node, *res;
    node = (unode_t *)dir_node->internal;
    if (node->type != UARTFS_TYPE_DIR) {
        uart_send_string("dir_node is not directory\r\n");
        return -1;
    };
    list_node_t *head = &node->head;
    // list_for_each(entry, head) {
    //     res = list_entry(entry, struct uartfs_node, head);
    // }
    for (list_node_t *entry = head->next; entry != head; entry = entry->next) {
        res = list_entry(entry, unode_t, head);
        if (strmatch(res->name, name)) {
            *target = res->node;
        }
    };
    return 1;
}

/* make directory under dir_node */
int uartfs_mkdir(vnode_t *dir_node, vnode_t **target, char *dirname) {
    unode_t *cur_node = dir_node->internal;
    if (cur_node->type != UARTFS_TYPE_DIR) {
        uart_send_string("dir_node is not directory\r\n");
        return -1;
    };

    unode_t *new_dir = (unode_t *)kmalloc(sizeof(unode_t));
    strcopy(new_dir->name, dirname);
    new_dir->type = UARTFS_TYPE_DIR;
    list_init(&new_dir->head);
    vnode_t *new_node = (vnode_t *)kmalloc(sizeof(vnode_t));
    new_node->mount = 0;
    new_node->parent = dir_node;
    new_node->internal = new_dir;
    new_node->v_ops = &UARTFS_VNODE_OPS;
    new_node->f_ops = &UARTFS_FILE_OPS;

    new_dir->node = new_node;
    list_add_tail(&new_dir->head, &cur_node->head);
    uart_send_string("uartfs_mkdir: create folder '");
    uart_send_string(new_dir->name);
    uart_send_string(" under ");
    uart_send_string(cur_node->name);
    uart_send_string("\r\n");
    *target = new_node;
    return 1;
}

/* create a new device node under dir_node */
int uartfs_mknode(vnode_t *dir_node, vnode_t **target, char *name) {
    uart_send_string("in uartfs_mknode\r\n");
    while(1){};
    unode_t *cur_node = dir_node->internal;
    if (cur_node->type != UARTFS_TYPE_DIR) {
        uart_send_string("dir_node is not directory\r\n");
        return -1;
    };

    unode_t *new_unode = (unode_t *)kmalloc(sizeof(unode_t));
    strcopy(new_unode->name, name);
    new_unode->type = UARTFS_TYPE_DIR;
    list_init(&new_unode->head);

    vnode_t *new_node = (vnode_t *)kmalloc(sizeof(vnode_t));
    new_node->mount = 0;
    new_node->parent = dir_node;
    new_node->internal = new_unode;
    new_node->v_ops = &UARTFS_VNODE_OPS;
    new_node->f_ops = &UARTFS_FILE_OPS;

    new_unode->node = new_node;
    list_add_tail(&new_unode->head, &cur_node->head);
    *target = new_node;
    return 1;
}

int uartfs_write(file_t *file, void *buf, unsigned int len) {
    uart_send_string("in uartfs_write\r\n");
    uart_send_string(buf);
    return 1;
}

int uartfs_read(file_t *file, void *buf, unsigned int len) {
    char *buffer = (char *)buf;
    while(len--) {
        *buffer = uart_read_char();
        buffer++;
    };
    return 1;
}

/* initialize uart file system and the pointer to it */
fs_t *uartfs_init() {
    vnode_t *UARTFS_ROOT = (vnode_t *)kmalloc(sizeof(vnode_t));
    UARTFS_ROOT->mount = 0;
    UARTFS_ROOT->parent = 0;
    UARTFS_ROOT->v_ops = &UARTFS_VNODE_OPS;
    UARTFS_ROOT->f_ops = &UARTFS_FILE_OPS;

    unode_t *root = (unode_t *)kmalloc(sizeof(unode_t));
    strcopy(root->name, "uart");
    root->type = UARTFS_TYPE_FILE;
    list_init(&root->head);
    root->node = UARTFS_ROOT;
    UARTFS_ROOT->internal = root;

    return &UARTFS;
}

/* make device node under dir_node */
int uartfs_mknod(vnode_t *dir_node, char *devname) {
    vnode_t *new_node;
    dir_node->v_ops->create(dir_node, &new_node, devname);
    new_node->f_ops = &UARTFS_FILE_OPS;
    return 1;
}
