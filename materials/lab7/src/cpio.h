#include "vfs.h"
#include "list.h"
/* in qemu environment */
#define CPIO_ADDR  (char *)0x8000000;

/* in raspberry pi environment */
//#define CPIO_ADDR  (char *)0x20000000;

// extern char* CPIO_ADDR;

/* the New ASCII Format 
 * https://man.freebsd.org/cgi/man.cgi?query=cpio&sektion=5
 */
struct cpio_header {
    /* uses 8-bytes hexadecimal fields for all numbers */
    char c_magic[6];     // determine whether this archive is written with little-endian or big-endian integers.
    char c_ino[8];       // determine when two entries refer to the same file.
    char c_mode[8];      // specifies both the regular permissions and the file type.
    char c_uid[8];       // numeric user id
    char c_gid[8];       // numeric group id
    char c_nlink[8];     // number of links to this file.
    char c_mtime[8];     // Modification time of the file
    char c_filesize[8];  // size of the file
    char c_devmajor[8];
    char c_devminor[8];
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8];  // number of bytes in pathname
    char c_check[8];     // always set to zero by writers and ignored by	readers.
};

struct cpio_file {
    const char *data;
    unsigned long size;
};

struct cpio_dir {
    list_node_t head; /* list of files in this directory */
};

/* file node in cpio file system */
typedef struct cnode {
    char name[16];    /* name of cnode, could be file name or directory name */
    int type;         /* indiate file or directory */
    struct cpio_file file;
    vnode_t *node; /* pointer to vnode */
    list_node_t list; /* list of file */
} cnode_t;


fs_t *cpiofs_init();
void cpio_ls();
void cpio_cat(char* fname);
void cpio_exec_prog(char* fname);
int cpio_find_file(char *fname, char **ret);
int cpiofs_mount(fs_t *fs, struct mount *mount);
int cpiofs_lookup(vnode_t *dir_node , vnode_t **target, char *name);
