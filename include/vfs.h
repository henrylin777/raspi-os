#pragma once
#include "list.h"
#include "arm.h"

/* similiar with dentry in Linux */
typedef struct vnode {
    char name[16];                  /* file name */
    struct mount *mount;            /* pointer to the mount point */
    struct vnode *parent;           /* parent directory */
    struct vnode_operations *v_ops; /* vnode operations */
    struct file_operations *f_ops;  /* file operations */
    list_node_t d_child;           /* child of parent's directory list */
    list_node_t d_subdirs;         /* list of child vnodes */
    void *internal;                 /* pointer to the node of physical file system */
} vnode_t;

/* file descriptor */
typedef struct file {
    struct vnode *vnode;            /* pointer to vnode */
    unsigned int f_pos;             /* RW position of this file handle */
    struct file_operations *f_ops;
    int flags;
} file_t;

/* struct mount representes a mounted file system */
typedef struct mount {
    vnode_t *root;     /* the foot vnode of file system */
    struct filesystem *fs;  /* the mounted file system */
    list_node_t list;       /* list of mount pointts */
} mount;

typedef struct filesystem {
    char name[16];
    vnode_t *root;
    list_node_t node; /* list of registered file systems */
    int (*mount)(struct filesystem *fs, struct mount *mount); /* mount operations*/
} fs_t;

typedef struct file_operations {
    int (*write)(file_t *fd, void *buf, unsigned int len);
    int (*read)(file_t *fd, void *buf, unsigned int len);
    int (*open)(struct vnode *file_node, struct file **target);
    int (*close)(file_t *fd);
    int (*cat)(struct vnode *file_node);
    // long lseek64(struct file *file, long offset, int whence);
} file_ops_t;

typedef struct vnode_operations {
    int (*lookup)(vnode_t *dir_node, vnode_t **target, char *name);
    int (*create)(vnode_t *dir_node, vnode_t **target, char *name);
    int (*mkdir)(vnode_t *dir_node, vnode_t **target, char *name);
} vnode_ops_t;
 

int vfs_open(char *pathname);
int vfs_read(int fd, void *buf, unsigned int len);
int vfs_close(int fd);
int vfs_write(int fd, char *buf, unsigned int len);

int vfs_chdir(char *path);
void vfs_init();
void vfs_register_fs(struct filesystem *fs);
int vfs_mkdir(char *name);
int vfs_mount(char *path, char *fsname);
int vfs_umount(char *path);
int vfs_lookup(char *path, vnode_t **target);
int vfs_create(char *fpath);
void vfs_init_rootmount(struct filesystem *fs);
void vfs_mknod(char *fpath, char *type);
void vfs_pwd();
void vfs_ls();

/* system call interface */
void syscall_open(trapframe_t *regs, char *fpath);
void syscall_close(trapframe_t *regs, int fd);
void syscall_write(trapframe_t *regs, int fd, char *buf, unsigned int len);
void syscall_read(trapframe_t *regs, int fd, char *buf, unsigned int len);
void syscall_mkdir(trapframe_t *regs, char *dirname);
void syscall_chdir(trapframe_t *regs, char *dirname);
void syscall_mount(trapframe_t *regs, char *path, char *fsname);
void syscall_chdir(trapframe_t *regs, char *dirname);
