#include <vfs.h>
#include <malloc.h>
#include <list.h>
#include <cpio.h>
#include <tmpfs.h>
#include <utils.h>
#include <uart.h>
#include <sche.h>
#include <arm.h>
#include <uartfs.h>
#include <kprintf.h>

list_node_t FS_HEAD; /* list of registered file systems */
list_node_t VFS_MOUNT_LIST; /* list of mount points */
vnode_t *VFS_ROOT;

int _find_mount(vnode_t **vnode) {
    if ((*vnode)->mount == 0) {
        return 0;
    };
    vnode_t *root = (*vnode)->mount->root;
    kprintf("switch file system\r\n");
    *vnode = root;
    return 1;
};

/* search file with 'dirname' under 'dir_node' and return the pointer to the node
 * return 0 if no such file
 */
vnode_t *_lookup_vnode(vnode_t *dir_node, char *dirname) {
    vnode_t *node, *target;
    target = 0;
    list_node_t *head, *cur;
    cur = head = &dir_node->d_subdirs;
    list_for_each(cur, head) {
        node = list_entry(cur, vnode_t, d_child);
        if (strmatch(node->name, dirname)) {
            target = node;
        };
    };
    return target;
}

vnode_t *_do_path_lookup(char *fpath) {
    char buf[16];
    vnode_t *cur_node, *next = 0;
    char *end;
    if (*fpath == '/') {
        cur_node = VFS_ROOT;
        fpath++;
    } else {
        task_struct *proc = get_current_proc();
        cur_node = proc->workdir;
    };
    while (*fpath) {
        if (strncmp("../", fpath, 3)) {
            if (cur_node->parent)
                cur_node = cur_node->parent;
            fpath += 3;
            continue;
        };
        if (strncmp("./", fpath, 2)) {
            if (cur_node->parent)
                cur_node = cur_node->parent;
            fpath += 2;
            continue;
        };
        end = fpath;
        do {
            end++;
        } while(*end && (*end != '/'));
        strncopy(buf, fpath, end - fpath);
        next = _lookup_vnode(cur_node, buf);
        if (!next) {
            kprintf("_do_path_lookup: no _lookup_vnode\r\n");
            return 0;
        }
        _find_mount(&next);
        cur_node = next;
        fpath = end;
    };
    return cur_node;
}

vnode_t *_get_dir_vnode(char **fpath) {

    char *start, *end; /* the range of file name */
    char buf[16];
    start = end = *fpath;
    do {
        while(*end && *end != '/')
            end++;
        start = end;
        end++;
    } while(*end);
    start++;
    strncopy(buf, *fpath, *fpath - start);
    vnode_t *dir_node = _do_path_lookup(buf);
    if (!dir_node) {
        kprintf("_get_dir_vnode: cannot found dir_node\r\n");
        return 0;
    }
    *fpath = start;
    return dir_node;

} 


fs_t *vfs_get_filesystem(char *fsname) {
    list_node_t *cur;
    fs_t *fs = 0;
    list_for_each(cur, &FS_HEAD) {
        fs = list_entry(cur, fs_t, node);
        if (strmatch(fs->name, fsname))
            break;
    };
    return fs;
}

/* mount file system on the path */
int vfs_mount(char *path, char *fsname) {
    vnode_t *mpoint;
    fs_t *fs;
    fs = vfs_get_filesystem(fsname);
    if (fs == 0) {
        uart_send_string("vfs_mount: no registed file system with name");
        uart_send_string(fsname);
        uart_send_string("\r\n");
        return -1;
    }
    mpoint = _do_path_lookup(path);
    if (!mpoint) {
        kprintf("vfs_mount: no such directory '%s'\r\n", path);
        return -1;
    };
    kprintf("vfs_mount: trying to mount '%s' at '%s'\r\n", fs->name, path);
    struct mount *mount = (struct mount *)kmalloc(sizeof(struct mount));
    mount->fs = fs;
    mount->root = mpoint;
    fs->mount(fs, mount);
    mpoint->mount = mount;
    list_add_tail(&mount->list, &VFS_MOUNT_LIST);
    uart_send_string("vfs_mount finished\r\n");
    /* todo: should remove the node of old file system ??? */
    return 1;
};

int vfs_umount(char *fpath) {
    struct mount *mnt, *tmp;
    char *dirname = fpath;
    task_struct *proc = get_current_proc();
    vnode_t *mnt_point = _lookup_vnode(proc->workdir, dirname);
    if (!mnt_point) {
        return -1;
    }
    mnt = mnt_point->mount;
    tmp = mnt;
    list_remove(&mnt->list);
    kfree(tmp);
    mnt_point->mount = 0;
    kprintf("vfs_umount: umount file system on '%s'\r\n", fpath);
    return -1;

}


/* set file system as root file system */
void vfs_init_rootmount(fs_t *fs) {
    struct mount *mount = (struct mount *)kmalloc(sizeof(struct mount));
    mount->fs = fs;
    mount->root = VFS_ROOT;
    list_init(&mount->list);
    fs->mount(fs, mount);
}


/* create a directory vnode under working directory */
int vfs_mkdir(char *dirname) {
    int ret;
    vnode_t *workdir, *new_dir;
    task_struct *cur = get_current_proc();
    workdir = cur->workdir;
    if (!workdir) {
        kprintf("vfs_mkdir: working directory is 0\r\n");
        return 0;
    };
    ret = workdir->v_ops->mkdir(workdir, &new_dir, dirname);
    list_add_tail(&new_dir->d_child, &workdir->d_subdirs);
    kprintf("vfs_mkdir: make directory '%s' under '%s'\r\n", dirname, workdir->name);
    return ret;
}


/* initialize the list head of all file systems */
void vfs_init() {
    VFS_ROOT = kmalloc(sizeof(vnode_t));
    VFS_ROOT->name[0] = '/';
    VFS_ROOT->name[1] = '\0';
    VFS_ROOT->parent = 0;
    list_init(&VFS_ROOT->d_child);
    list_init(&VFS_ROOT->d_subdirs);
    task_struct *cur = get_current_proc();
    cur->workdir = VFS_ROOT;

    list_init(&FS_HEAD);
    list_init(&VFS_MOUNT_LIST);
}

void vfs_register_fs(struct filesystem *fs) {
    list_add_tail(&fs->node, &FS_HEAD);
}

/* make a special file (e.g. device) */
void vfs_mknod(char *fpath, char *type) {
    /* TODO: finish mknod */
    // char *fname = fpath;
    // vnode_t *dir_node;
    // dir_node = _get_dir_vnode(VFS_ROOT_MOUNT->root, &fname);
    task_struct *proc = get_current_proc();
    if (*type)
        uartfs_mknod(proc->workdir, fpath);
}




void do_pwd_traverse(vnode_t *root,vnode_t *cur) {
    if (root != cur->parent)
        do_pwd_traverse(root, cur->parent);
    kprintf("/%s", cur->name);
}

void vfs_pwd() {
    task_struct *cur = get_current_proc();
    vnode_t *node = cur->workdir;
    if (!node) {
        uart_send_string("vfs_pwd: work directory is not set\r\n");
        while(1){};
    };
    if (node->parent == 0) {
        uart_send_string("/\r\n");
        return;
    }
    do_pwd_traverse(VFS_ROOT, node);
    uart_send_string("\r\n");
}

void vfs_ls() {
    task_struct *proc = get_current_proc();
    vnode_t *node, *entry;
    node = proc->workdir;
    if (!node) {
        uart_send_string("vfs_pwd: work directory is not set\r\n");
        while(1){};
    };
    list_node_t *cur, *head;
    cur = head = &node->d_subdirs;
    list_for_each(cur, head) {
        entry = list_entry(cur, vnode_t, d_child);
        kprintf("  %s\r\n", entry->name);
    };
    return;
}

/* set working directory to path */
int vfs_chdir(char *dirname) {
    vnode_t *target;
    target = _do_path_lookup(dirname);
    /* TODO: find file in disk */
    if (!target) {
        kprintf("vfs_chdir: directory '%s' not found\r\n", dirname);
        return -1;
    }
    task_struct *proc = get_current_proc();
    proc->workdir = target;
    kprintf("vfs_chdir: change working directory to '%s'\r\n", dirname);
    return 1;
}

/* create a file hander and set it to the entry of file scriptor table of current process  
 * On success, return the index of file handle in the file descriptor table 
 * On fail, return -1
 */
int vfs_open(char *fname) {
    task_struct *cur = get_current_proc();
    /* find the index of the current process */
    int idx;
    for(idx = 0; idx < THREAD_MAX_FD; idx++) {
        if (cur->fdtable[idx].vnode == 0)
            break;
    };
    if (idx >= THREAD_MAX_FD) {
        kprintf("open too many files\r\n");
        return -1;
    };
    file_t *handle = &cur->fdtable[idx];
    vnode_t *dir_node = cur->workdir;
    vnode_t *new_node;
    if (dir_node->v_ops->lookup(dir_node, &new_node, fname) == -1) {
        kprintf("vfs_open: file '%s'is not under '%s'\r\n", fname, dir_node->name);
        return -1;
    }
    new_node->f_ops->open(new_node, &handle);
    cur->numfd++;
    kprintf("vfs_open: file opened\r\n");
    return idx;
}

/* write data into file 
 * On success will return the write size
 * On failure will return -1
 */
int vfs_write(int fd, char *buf, unsigned int len) {
    task_struct *cur = get_current_proc();
    file_t *handle = &cur->fdtable[fd];
    return handle->f_ops->write(handle, buf, len);
}

/* read file content with len bytes and write it to buffer 
 * On success will return the read size
 * On failure will return -1 
 */
int vfs_read(int fd, void *buf, unsigned int len) {
    task_struct *cur = get_current_proc();
    file_t *handle = &cur->fdtable[fd]; /* file handler */
    return handle->f_ops->read(handle, buf, len);
}

/* create a file and return the file descriptor */
int vfs_create(char *fname) {
    task_struct *proc = get_current_proc();
    vnode_t *dir_node = proc->workdir;
    if (!dir_node) {
        kprintf("vfs_create: no dir_node\r\n");
        return -1;
    };
    vnode_t *new_node;
    if (dir_node->v_ops->create(dir_node, &new_node, fname) == -1) {
        uart_send_string("vfs_create: create failed\r\n");
        return -1;
    }

    int idx;
    task_struct *cur = get_current_proc();
    for(idx = 0; idx < THREAD_MAX_FD; idx++) {
        if (cur->fdtable[idx].vnode == 0)
            break;
    };
    file_t *handle = &cur->fdtable[idx];
    new_node->f_ops->open(new_node, &handle);
    cur->numfd++;
    kprintf("vfs_create: file '%s' create and opened\r\n", fname);
    return idx;

}

/* close the file 
 * On success will return 0 and the vnode of file handler will be set to none
 * On failure will return -1
 */
int vfs_close(int fd) {
    task_struct *cur = get_current_proc();
    if (fd < 0 || fd > cur->numfd)
        return -1;
    file_t *handle = &cur->fdtable[fd];
    int res = handle->f_ops->close(handle);
    cur->numfd--;
    return res;
}

/* write frame buffer again without re-open */
// int vfs_lseek64() {}

/* system call for opening a file for current process 
 * On success, will return the index of file scriptor of the current process
 * On failure, will do nothing and return -1;
 */
void syscall_open(trapframe_t *regs, char *fpath) {
    int res = vfs_open(fpath);
    regs->x0 = res;
    // regs->x0 = 0;
}

void syscall_close(trapframe_t *regs, int fd) {
    int res = vfs_close(fd);
    regs->x0 = res;
}

void syscall_write(trapframe_t *regs, int fd, char *buf, unsigned int len) {
    int res = vfs_write(fd, buf, len);
    regs->x0 = res;
}

void syscall_read(trapframe_t *regs, int fd, char *buf, unsigned int len) {
    int res = vfs_read(fd, buf, len);
    regs->x0 = res;
}

void syscall_mkdir(trapframe_t *regs, char *dirname) {
    int res = vfs_mkdir(dirname);
    regs->x0 = res;
}

/* mount the registered file system on path 
 * On success will return 0
 * On failure will return -1
 */
void syscall_mount(trapframe_t *regs, char *path, char *fsname) {
    int res = vfs_mount(path, fsname);

    regs->x0 = res;
}

void syscall_chdir(trapframe_t *regs, char *dirname) {
    int res = vfs_chdir(dirname);
    regs->x0 = res;
}
