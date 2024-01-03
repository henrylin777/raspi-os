#include "vfs.h"
#include "tmpfs.h"
#include "utils.h"
#include "malloc.h"
#include "uart.h"
#include "list.h"
#include "sche.h"
#include "kprintf.h"

#define UNUSED(x) (void)(x)
int tmpfs_mount(fs_t *fs, struct mount *mount);
int tmpfs_create(vnode_t *dir_node, vnode_t **target, char *name);

int tmpfs_open(vnode_t *file_node, struct file **target);
int tmpfs_write(file_t *fd, void *buf, unsigned int len);
int tmpfs_read(file_t *fd, void *buf, unsigned int len);
int tmpfs_close(file_t *fd);

static struct filesystem TMPFS = {
    .name = "tmpfs",
    .mount = tmpfs_mount,
};

static file_ops_t TFS_FOPS = {
    .open = tmpfs_open,
    .close = tmpfs_close,
    .read = tmpfs_read,
    .write = tmpfs_write,
};

tfs_node_t TFS_ROOT;

static vnode_ops_t TFS_VOPS = {
    .lookup = tmpfs_lookup,
    .create = tmpfs_create,
    .mkdir = tmpfs_mkdir,
};

int tmpfs_open(vnode_t *file_node, struct file **target) {
    file_t *t = *target;
    t->vnode = file_node;
    t->f_pos = 0;
    t->f_ops = file_node->f_ops;
    return 1;
}

int tmpfs_close(file_t *fd) {
    fd->vnode = 0;
    fd->f_pos = 0;
    fd->f_ops = 0;
    return 1;
}

int tmpfs_write(file_t *file, void *buf, unsigned int len) {
    tfs_node_t *node = file->vnode->internal;
    strncopy(node->data, buf, len);
    return 1;
}

int tmpfs_read(file_t *file, void *buf, unsigned int len) {
    tfs_node_t *node = file->vnode->internal;
    strncopy(buf, node->data, len);
    return len;
}

// /* make a directory under dir_node */
int tmpfs_mkdir(vnode_t *dir_node, vnode_t **target, char *name) {
    tfs_node_t *new_tnode;
    vnode_t *new_vnode;

    new_vnode = kmalloc(sizeof(vnode_t));
    new_tnode = kmalloc(sizeof(tfs_node_t));
    
    new_tnode->type = TMPFS_TYPE_DIR;
    new_tnode->vfs_node = new_vnode;

    strcopy(new_vnode->name, name);
    list_init(&new_vnode->d_subdirs);
    new_vnode->parent = dir_node;
    new_vnode->mount = 0;
    new_vnode->internal = new_tnode;
    new_vnode->parent = dir_node;
    new_vnode->v_ops = &TFS_VOPS;
    new_vnode->f_ops = &TFS_FOPS;
    kprintf("tmpfs_mkdir: make directory '%s' under '%s'\r\n", name, dir_node->name);
    *target = new_vnode;
    return 0;
}

int tmpfs_mount(fs_t *fs, struct mount *mount){
    tfs_node_t *root = kmalloc(sizeof(tfs_node_t));
    root->type = TMPFS_TYPE_DIR;
    vnode_t *vnode = mount->root;
    root->vfs_node = vnode;
    vnode->internal = root;
    vnode->f_ops = &TFS_FOPS;
    vnode->v_ops = &TFS_VOPS;
    kprintf("tmpfs_mount: mount tmpfs at '%s'\r\n", mount->root->name);
    return 1;
}

fs_t *tmpfs_init() {
    return &TMPFS;
}

/* get the node with name under dir_node, and set target to it */
int tmpfs_lookup(vnode_t *dir_node, vnode_t **target, char *name) {
    return -1;
    // uart_send_string("tmpfs_lookup: trying to get the node with name '");
    // uart_send_string(name);
    // uart_send_string("' under directory ");
    // tfs_node_t *tmp = dir_node->internal;
    // uart_send_string(tmp->name);
    // uart_send_string("\r\n");
    // tfs_node_t *internal, *cur;
    // internal = dir_node->internal;
    // list_node_t *head = &internal->dir.head;

    // for (list_node_t *entry = head->next; entry != head; entry = entry->next) {
    //     cur = list_entry(entry, tfs_node_t, list);
    //     if (strmatch(cur->name, name)) {
    //         *target = cur->oldnode;
    //         // uart_send_string("got file\r\n");
    //         return 1;
    //     }
    // };
    // uart_send_string("no such file\r\n");
    // return -1;
}

/* create a file node with name under dir_node */
int tmpfs_create(vnode_t *dir_node, vnode_t **target, char *name)
{
    tfs_node_t *tmp = dir_node->internal; 
    if (tmp->type != TMPFS_TYPE_DIR) {
        kprintf("tmpfs_create: file '%s' is not directory\r\n", dir_node->name);
        while(1){};
    };

    tfs_node_t *new = (tfs_node_t *)kmalloc(sizeof(tfs_node_t));
    new->type = TMPFS_TYPE_FILE;
    vnode_t *new_vnode = (vnode_t *)kmalloc(sizeof(vnode_t));
    kprintf("debug\r\n");
    strcopy(new_vnode->name, name);
    memset(new->data, 0, 16);
    new_vnode->v_ops = &TFS_VOPS;
    new_vnode->f_ops = &TFS_FOPS;
    new_vnode->internal = new;
    new_vnode->mount = 0;
    new_vnode->parent = dir_node;
    list_init(&new_vnode->d_subdirs);
    list_init(&new_vnode->d_child);
    list_add_tail(&new_vnode->d_child, &dir_node->d_subdirs);
    new->vfs_node = new_vnode;


    kprintf("tmpfs_create: create a file '%s' under '%s'\r\n", name, dir_node->name);
    *target = new_vnode;
    return 1;
}
