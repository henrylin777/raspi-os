#include "cpio.h"
#include "uart.h"
#include "utils.h"
#include "malloc.h"
#include "vfs.h"
#include "list.h"

#define CPIO_TYPE_MASK     0170000
#define CPIO_TYPE_DIR      0040000
#define CPIO_TYPE_FILE     0100000 
#define UNUSED(x) (void)(x)

int cpiofs_mkdir(struct vnode *dir_node, struct vnode **target, char *component_name);
int cpiofs_lookup(struct vnode *dir_node, struct vnode **target, char *component_name);
int cpiofs_getname(struct vnode *node, char **name);
int cpiofs_cat(struct vnode *node);
int cpiofs_read(file_t *fd, void *buf, unsigned int len);
int cpiofs_close(file_t *fd);
int cpiofs_write(file_t *fd, void *buf, unsigned int len);
int cpiofs_ls(vnode_t *node);
int cpiofs_create(vnode_t *dir_node, vnode_t **target, char *fname);
int cpiofs_chdir(vnode_t *node, vnode_t **target, char *dirname);

fs_t CPIO_FS = {
    .name = "cpio",
    .mount = cpiofs_mount,
};

static vnode_ops_t CPIO_VNODE_OPS = {
    .lookup = cpiofs_lookup,
    .create = cpiofs_create,
    .mkdir = cpiofs_mkdir,
};

static file_ops_t CPIO_FILE_OPS = {
    .cat = cpiofs_cat,
    .read = cpiofs_read,
    .write = cpiofs_write,
    .close = cpiofs_close,
};

vnode_t CPIO_ROOT_NODE;

unsigned long HEADER_SIZE = sizeof(struct cpio_header);

/* 
    cpio archive structure 
    +-----------------+ 
    | file1 header    | 
    +-----------------+
    | file1 path name |
    +-----------------+
    | (PADDING)       |
    +-----------------+
    | file1 content   |
    +-----------------+ 
    | (PADDING)       | 
    +-----------------+
    | file2 header    |
    +-----------------+ 
    | (...)           | 
    +-----------------+
    | TRAILER!!!      |
    +-----------------+
*/   
   
unsigned long cpio_hex2int(const char *str) {
    unsigned long num = 0;
    for (int i = 0; i < 8; i++) {
        num = num * 16;
        if (*str >= '0' && *str <= '9') {
            num += (*str - '0');
        } else if (*str >= 'A' && *str <= 'F') {
            num += (*str - 'A' + 10);
        } else if (*str >= 'a' && *str <= 'f') {
            num += (*str - 'a' + 10);
        }
        str++;
    }
    return num;
}

void cpio_namecpy(char *array, char *fname) {
    while(*fname) {
        *array = *fname;
        fname++; array++;
    }
    *array = '\0';
}


/* get the file object with fname from cpio arvhive and return the pointer to the head of file.
 * return 0 if not found 
*/
int cpio_find_file(char *fname, char **ret)
{
    unsigned long headpath_size, file_size;
    file_size = 0;
    char *ptr = CPIO_ADDR;
    char *pname = ptr + HEADER_SIZE;
    struct cpio_header* pheader;
    while (strmatch(pname, "TRAILER!!!") == 0) {
        if ((strmatch(pname, fname) != 0)) {
            *ret = ptr;
            return file_size;
        }
        pheader = (struct cpio_header *)ptr;
        headpath_size = cpio_hex2int(pheader->c_namesize) + HEADER_SIZE;
        file_size =  cpio_hex2int(pheader->c_filesize);
        ptr += (alignto(headpath_size, 4) + alignto(file_size, 4));
        pname = ptr + HEADER_SIZE;
    };
    return file_size;
}



/* print the content of the file (like command `cat`) */
void cpio_cat(char *fname)
{
    unsigned long offset;
    struct cpio_header* phead;
    char *ptr;
    if (cpio_find_file(fname, &ptr)) {
        phead = (struct cpio_header *) ptr;
        offset = cpio_hex2int(phead->c_namesize) + HEADER_SIZE;
        ptr += offset;
        uart_send_string(ptr);
        uart_send_char('\n');
        return;
    };
    uart_send_string("cpio_cat: ");
    uart_send_string(fname);
    uart_send_string(": No such file or directory!\n");
}

/* list out all files in cpio archives (similiar with `ls`) */
void cpio_ls(){
	char *ptr = CPIO_ADDR;
    unsigned long fname_size, file_size;
    struct cpio_header *header;

    /* parse files until reach the end 'TRAILER!!!' */
	while(strmatch((char *)(ptr + HEADER_SIZE), "TRAILER!!!") == 0) {
		/* print file name until reach null char */
		uart_send_string(ptr + HEADER_SIZE);
		uart_send_string("\n");

		header = (struct cpio_header*) ptr;
		fname_size = cpio_hex2int(header->c_namesize);
		fname_size += HEADER_SIZE;
		file_size = cpio_hex2int(header->c_filesize);
		ptr += (alignto(fname_size, 4) + alignto(file_size, 4));
	}
}

/* load file from cpio archive, switch from el1 to el0 and execute */
void cpio_exec_prog(char* fname) {
    unsigned int spsr_el1;
    unsigned long jump_addr, offset, fsize;
    char *addr = (char *) 0x20000;
    char *pfile;
    
    if (!cpio_find_file(fname, &pfile)) {
        uart_send_string("file not found!\n");
        return;
    }
    struct cpio_header *phead = (struct cpio_header *)pfile;
    offset = cpio_hex2int(phead->c_namesize) + HEADER_SIZE;
    fsize = cpio_hex2int(phead->c_filesize);

    pfile += offset;
	spsr_el1 = 0x3c0;
    jump_addr = 0x20005;

	while(fsize--)
		*addr++ = *pfile++;
    /* switch from el1 to el0 */
    unsigned long sp = (unsigned long) kmalloc(0x2000);
	asm volatile("msr spsr_el1, %0" ::"r"(spsr_el1));
	asm volatile("msr elr_el1, %0" ::"r"(jump_addr));
	asm volatile("msr sp_el0, %0" :: "r"(sp));
    /* jumo to el0 and execute file */
    
	asm volatile("eret");
    
}

/* find the node of name under dir_node and set it to target 
 * On success, target will be set to the node and return 1
 * On failure, target will not be set and return -1
 */
int cpiofs_lookup(vnode_t *dir_node , vnode_t **target, char *name) {
    cnode_t *internal;
    internal = dir_node->internal;
    uart_send_string("cpiofs_lookup: trying to find file '");
    uart_send_string(name);
    uart_send_string("' under ");
    uart_send_string(internal->name);
    uart_send_string("\r\n");
    if (internal->type != CPIO_TYPE_DIR) {
        uart_send_string("lookup: ");
        uart_send_string(name);
        uart_send_string(" is not directory\r\n");
        return 0;
    }
    cnode_t *cur;
    list_node_t *head = &internal->list;
    if (list_empty(head)) {
        uart_send_string("directory ");
        uart_send_string(internal->name);
        uart_send_string(" is empty\r\n");
        return 0;
    }
    // list_node_t *entry = head->next;
    // cur = list_entry(entry, cnode_t, list);
    // uart_send_string(&cur->name[0]);
    // while(1){};
    for (list_node_t *entry = head->next; entry != head; entry = entry->next) {
        cur = list_entry(entry, cnode_t, list);
        if (strmatch(cur->name, name)) {
            *target = cur->node;
            return 1;
        }
    };
    return -1;
}

int cpiofs_mkdir(struct vnode *dir_node, struct vnode **target, char *name)
{
    cnode_t *cur_node, *newdir;
    list_node_t *head;
    vnode_t *new_node;

    cur_node = dir_node->internal;
    head = &cur_node->list;

    new_node = (vnode_t *)kmalloc(sizeof(vnode_t));
    newdir = (cnode_t *)kmalloc(sizeof(cnode_t));
    

    strcopy(newdir->name, name);
    newdir->type = CPIO_TYPE_DIR;
    newdir->node = new_node;

    new_node->mount = 0;
    new_node->parent = dir_node;
    new_node->internal = newdir;
    new_node->v_ops = &CPIO_VNODE_OPS;
    new_node->f_ops = 0;
    uart_send_string("cpio_mkdir: make directory '");
    uart_send_string(newdir->name);
    uart_send_string("' at ");
    uart_send_string(cur_node->name);
    uart_send_string("\r\n");
    list_add_tail(&newdir->list, head);
    *target = new_node;
    return 1;
}

void cpiofs_init_node(char *fpath, char *content, unsigned int size) {
    // vnode_t *dir_node = get_dir_vnode(&CPIO_ROOT_NODE, &fpath);
    vnode_t *dir_node = &CPIO_ROOT_NODE;
    cnode_t *parent;

    if (!dir_node) {
        uart_send_string("no dir_node\r\n");
        return;
    }

    cnode_t *new_cnode = (cnode_t *)kmalloc(sizeof(cnode_t));
    vnode_t *new_vnode = (vnode_t *)kmalloc(sizeof(vnode_t));

    parent = (cnode_t *)dir_node->internal;
    cpio_namecpy(&new_cnode->name[0], fpath);
    new_cnode->type = CPIO_TYPE_FILE;
    new_cnode->file.data = content;
    new_cnode->file.size = size;
    new_cnode->node = new_vnode;
    uart_send_string("cpiofs_init_node: insert cnode ");
    uart_send_string(new_cnode->name);
    uart_send_string(" into cnode: ");
    uart_send_string(parent->name);
    uart_send_string("\r\n");
    list_add_tail(&new_cnode->list, &parent->list);

    new_vnode->mount = 0;
    new_vnode->parent = dir_node;
    new_vnode->internal = new_cnode;
    new_vnode->f_ops = &CPIO_FILE_OPS;

    return;
}

/* initialize cpio file system and return the pointer to fs_t */
fs_t *cpiofs_init() {

    /* init root node */
    struct cnode *new_root = (struct cnode *)kmalloc(sizeof(struct cnode));
    cpio_namecpy(&new_root->name[0], "/");
    new_root->type = CPIO_TYPE_DIR;
    list_init(&new_root->list);
    new_root->node = &CPIO_ROOT_NODE;

    CPIO_ROOT_NODE.mount = 0;
    CPIO_ROOT_NODE.parent = 0;
    CPIO_ROOT_NODE.internal = new_root;
    CPIO_ROOT_NODE.v_ops = &CPIO_VNODE_OPS;

    unsigned long headpath_size, file_size, type;
    file_size = 0;
    char *ptr, *pname, *pfile;
    ptr = CPIO_ADDR;
    // char *pname = ptr + HEADER_SIZE;
    // char *pfile;
    struct cpio_header* pheader;
    /* variable
     * ptr   : pointer to the start of next file
     * pfile : pointer to the content of current file
     */
    while (1) {
        pheader = (struct cpio_header *)ptr;
        headpath_size = cpio_hex2int(pheader->c_namesize) + HEADER_SIZE;
        file_size     = cpio_hex2int(pheader->c_filesize);
        type          = cpio_hex2int(pheader->c_mode) & CPIO_TYPE_MASK;
        pname         = ptr + HEADER_SIZE;
        pfile         = (char *)pheader + cpio_hex2int(pheader->c_namesize) + HEADER_SIZE;
        if (strmatch(pname, "TRAILER!!!"))
            break;
        if (type == CPIO_TYPE_FILE) {
            cpiofs_init_node(pname, pfile, file_size);
            // cpiofs_lookup(&CPIO_ROOT_NODE, "/");
        }
        ptr += (alignto(headpath_size, 4) + alignto(file_size, 4));
    };
    list_init(&CPIO_FS.node);
    return &CPIO_FS;
}

struct vnode mount_old_node; /* backup old node */

/* mount cpio file system at mount->root which is given by parent file system 
 * fs is cpio file system,
 */
int cpiofs_mount(fs_t *fs, struct mount *mount) {
    uart_send_string("do cpiofs_mount\r\n");
    UNUSED(fs);

    // /* backup */
    // mount_old_node.mount = CPIO_ROOT_NODE.mount;
    // mount_old_node.v_ops = CPIO_ROOT_NODE.v_ops;
    // mount_old_node.f_ops = CPIO_ROOT_NODE.f_ops;
    // mount_old_node.parent = CPIO_ROOT_NODE.parent;
    // mount_old_node.internal = CPIO_ROOT_NODE.internal;

    // /* replace node with cpio root node */
    // vnode_t *oldnode = mount->root;
    // cnode_t *cpio_root = CPIO_ROOT_NODE.internal;
    // CPIO_ROOT_NODE.parent = oldnode->parent;
    // char *name;
    // oldnode->v_ops->getname(oldnode, &name);
    // cpio_namecpy(&cpio_root->name[0], name);
    
    // // node->v_ops = CPIO_ROOT_NODE.v_ops;
    // // node->internal = cpio_root;
    // // node->mount = mount;
    // mount->root = &CPIO_ROOT_NODE;
    // /* TODO: free node */
    return 1;
}

int cpiofs_getname(struct vnode *node, char **name) {
    cnode_t *n = node->internal;
    *name = n->name;
    return 1;
};

int cpiofs_cat(vnode_t *file) {
    uart_send_string("in cpiofs_cat\r\n");
    cnode_t *node = file->internal;
    struct cpio_file f = node->file;
    uart_send_string((char *)f.data);
    return 1;
}

int cpiofs_read(file_t *fd, void *buf, unsigned int len) {
    cnode_t *node = (cnode_t *)fd->vnode->internal;
    unsigned long size = node->file.size;
    int rem = fd->f_pos + len - size; 
    if (rem > 0) {
        len -= rem;
    };
    const char *pos = node->file.data + fd->f_pos;
    strncopy(buf, (char *)pos, len);
    fd->f_pos += len;
    return len;

}

int cpiofs_close(file_t *fd) {
    fd->vnode = 0;
    fd->f_pos = 0;
    fd->f_ops = 0;
    return 1;
}

int cpiofs_write(file_t *fd, void *buf, unsigned int len) {
    uart_send_string("file write operation is not supported in cpio file system\r\n");
    return -1;
}


int cpiofs_ls(vnode_t *node) {
    cnode_t *dir_node, *cur;
    dir_node = node->internal;
    if (dir_node->type != CPIO_TYPE_DIR) {
        uart_send_string("cpiofs_ls: node is not directory\r\n");
        return -1;
    };
    for (list_node_t *entry = (&dir_node->list)->next; entry != &dir_node->list; 
        entry = entry->next) {
        cur = list_entry(entry, cnode_t, list);
        uart_send_string("  ");
        uart_send_string(cur->name);
        uart_send_string("\r\n");
    };
    return 1;
}

int cpiofs_create(vnode_t *dir_node, vnode_t **target, char *fname) {
    uart_send_string("file create operation is not supported in cpio file system\r\n");
    return -1;
}

int cpiofs_chdir(vnode_t *node, vnode_t **target, char *dirname) {
    uart_send_string("DEBUG\r\n");
    if (strncmp("..", dirname, 2)) {
        *target = node->parent;
        return 1;
    }
    if (strncmp(".", dirname, 1)) {
        *target = node;
        return 1;
    }
    int ret;
    vnode_t *dir_node;
    ret = cpiofs_lookup(node, &dir_node, dirname);
    if (ret != 1)
        return ret;
    cnode_t *n = dir_node->internal;
    if (n->type != CPIO_TYPE_DIR) {
        uart_send_string(dirname);
        uart_send_string("is not directory\r\n");
        return -1;
    }
    *target = dir_node;
    return 1;
}
