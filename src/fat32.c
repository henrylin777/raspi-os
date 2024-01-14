#include <vfs.h>
#include <sdhost.h>
#include <fat32.h>
#include <malloc.h>
#include <utils.h>
#include <list.h>
#include <uart.h>
#include <kprintf.h>
#include <bitops.h>

/* fat file type */
#define FAT32_TYPE_DIR              1
#define FAT32_TYPE_FILE             2
#define NUM_ENTRY_FAT               (BLOCK_SIZE / sizeof(fat32_centry_t))
#define DIR_PER_BLOCK               (BLOCK_SIZE / sizeof(fat32_centry_t))
/* number of blocks per clutser in disk */
#define BLK_PER_CLUSTER             1
#define INVALID_CID                 0x0ffffff8

/* directory entry status */
#define DIR_ENTRY_LAST_AND_UNUSED 0x00 /* no later entry is allocated in this sequence */
#define DIR_ENTRY_UNUSED          0xE5 /* available entry */

/* directory attribute */
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_LFN        0x0f
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_FILE_DIR_MASK (ATTR_DIRECTORY | ATTR_ARCHIVE)

/* FAT entry value */
#define FAT_ENTRY_UNUSED            0x0
#define FAT_ENTRY_END               0x0FFFFFF9
#define FAT_USED_AND_END_OF_FILE    0xFFFFFF0F

#define entry_is_free(x) (x == FAT_ENTRY_UNUSED)
#define cid2lba(cid, fat) (cid - 2 + fat->data_lba)
#define eid2addr(eid) (eid * sizeof(fat32_centry_t))
#define invalid_cid(cid) (cid >= INVALID_CID)
#define fat32_isdir(node) (node->type == FAT32_TYPE_DIR)




static struct filesystem FAT32FS = {
    .name = "fat32fs",
    .mount = fat32_mount,
};

vnode_ops_t FAT32_VOPS = {
    .lookup = fat32_lookup,
    .create = fat32_create,
    .mkdir = fat32_mkdir,
};


file_ops_t FAT32_FOPS = {
    .read = fat32_read,
    .open = fat32_open,
    .write = fat32_write,
    .close =  fat32_close,
};

/* list of fat_mount_t */
list_node_t FAT32_MOUNT_LIST;
fat32_info_t *FAT32INFO;

#define MARK_BLOCK_USED(prev, lba)          \
({                                          \
    uint32 *table = FAT32INFO->table;       \
    table[prev] = lba;                      \
    table[lba] = FAT_USED_AND_END_OF_FILE;  \
})                                          \


#define MARK_BLOCK_UNUSED(prev, lba)        \
({                                          \
    uint32 *table = FAT32INFO->table;       \
    table[prev] = FAT_USED_AND_END_OF_FILE; \
    table[lba] = FAT_ENTRY_UNUSED;          \
})                                          \

#define MARK_BLOCK_END(lba)                 \
({                                          \
    uint32 *table = FAT32INFO->table;       \
    table[lba] = FAT_USED_AND_END_OF_FILE;  \
})                                          \


/* allocate a free cluster from FAT and return cid
 * return -1 if failure
 */
int _alloc_free_cluster() {
    uint32 ret;
    boot_sector_t bs = FAT32INFO->bs;
    uint32 num_blks = bs.cluster_size * bs.fat32_size * bs.fats;
    uint32 *table = FAT32INFO->table;
    table += 2;
    for (ret = 2; ret < num_blks; ret++, table++) {
        if (*table == FAT_ENTRY_UNUSED) {
            kprintf("_alloc_free_cluster: ret %d\r\n", ret);
            return ret;
        }
    }
    return 0;
}


vnode_t *_create_vnode(vnode_t *dir_node, fat32_node_t *node, char *fname) {
    
    vnode_t *new = kmalloc(sizeof(vnode_t));
    strcopy(new->name, fname);
    new->mount = dir_node->mount;
    new->parent = dir_node;
    new->v_ops = &FAT32_VOPS;
    new->f_ops = &FAT32_FOPS;
    list_init(&new->d_subdirs);
    list_init(&new->d_child);
    list_add_tail(&new->d_child, &dir_node->d_subdirs);
    new->internal = node;
    node->vfs_node = new;
    
    return new;
}

/* get the next clutser id from sd card (FAT region) */
uint32 get_next_cid(uint32 curcid) {
    kprintf("in get_next_cid\r\n");
    fat32_centry_t *centry;
    uint32 offset;
    uint8 buf[BLOCK_SIZE];
    offset = eid2addr(curcid);
    
    sd_read_block(FAT32INFO->fat_lba, buf);
    /* todo: cache fat */

    centry = (fat32_centry_t *)&buf[offset];
    // kprintf("get_next_cid: fat->fat_lba %d\r\n", fat->fat_lba);
    // kprintf("get_next_cid: offset %d\r\n", offset);
    kprintf("get_next_cid: return %d\r\n", centry->val);
    return centry->val;
}



/* seek data block and locate the target data block with bid from vfs, and set 'target' to it if found
 * target will be NULL if list is empty 
 * otherwise the target would be the tail block or the target block
 * return 1 if target is found, otherwise return -1
 * Note that 'bid' is the index of data block in block linked list.
 * 
 * @data:   the data node
 * @bid:    the target block id
 * @target: the target block
 */
int _seek_cache(fat32_node_t *data, uint32 bid, fat32_block_t **target) {
    kprintf("in _seek_cache\r\n");
    fat32_block_t *block = NULL;
    list_node_t *head, *cur;
    head = &data->list;

    if (list_empty(head)) return -1;

    list_for_each(cur, head) {
        block = list_entry(cur, fat32_block_t, list);
        if (bid == block->bid) {
            *target = block;
            kprintf("_seek_cache: target block with id %d found\r\n", bid);
            return 1;
        }
    };
    *target = block;
    kprintf("_seek_cache: no data block with id '%d' in node '%s'\r\n", bid);
    return -1;
}

/* load several cluster data from 'cid' from disk until the target block with 'tbid' is found
 * if found, 'tail_block' will be set to the target block and return 1
 * otherwise, 'tail_block' will be set to the tail block of the last cluster we load and return -1
 * 
 * @tbid:       the target block id
 * @cid:        the last clutser id (unused for now)
 * @tail_block: the tail data block in vfs
 */
int _load_fat32(fat32_node_t *data, uint32 tbid, uint32 cid,
                         fat32_block_t **tail_block)
{
    uart_send_string("in _load_fat32\r\n");
    kprintf("_load_fat32: target bid %d\r\n", tbid);
    fat32_block_t *newblock;
    uint32 curbid = 0;
    uint32 curcid = cid; /* current cluster id */
    uint8 buf[BLOCK_SIZE];
    int found = 0;
    
    if (*tail_block) { /* partical content is already in vfs */
        curbid = (*tail_block)->bid + 1;
        curcid = get_next_cid((*tail_block)->cid);
    };

    kprintf("_load_fat32: curbid %d\r\n", curbid);
    while (1) {
        uint32 clba = cid2lba(curcid, FAT32INFO);
        // kprintf("_load_fat32: clba %d\r\n", clba);
        for (uint32 i = 0; i < BLK_PER_CLUSTER; i++) {
            newblock = kmalloc(sizeof(fat32_block_t));
            newblock->bid = curbid;
            newblock->cid = curcid;
            newblock->dirty = 0;
            uint32 lba = clba + i;
            sd_read_block(lba, buf);
            kprintf("_load_fat32: data block with id %d is adding data from disk with lba %d\r\n", curbid, lba);
            memcopy((void *)newblock->buf, (void *)buf, BLOCK_SIZE);
            list_add_tail(&newblock->list, &data->list);
            if (curbid == tbid) {
                *tail_block = newblock;
                found = 1;
            }
            curbid++;
        };
        if (found) break;
        curcid = get_next_cid(curcid);
        kprintf("_load_fat32: the next cid %d\r\n", curcid);
        if (invalid_cid(curcid)) {
            uart_send_string("_load_fat32: invalid cid\r\n");
            return -1;
        };
    }
    return 1;
}

/* read buffer data from data block. Start from 'bufoff' and read 'size' bytes
 * On success return 1, otherwise return -1
 */
static int _read_cache(fat32_block_t *block,uint8 *buf, 
                       uint64 bufoff, uint32 rsize)
{
    memcopy((void *)&buf[0], (void *)&block->buf[bufoff], rsize);
    return 1;
}

/* get available directory entry under dir_node and return the offset in directory
 * return -1 if failure
 */
int _alloc_entry(fat32_node_t *dir_node) {
    uint32 cid = dir_node->cid;
    uint8 buf[512];
    sde_t *dentry;
    sd_read_block(cid2lba(cid, FAT32INFO), buf); /* read directory table block */
    /* traverse directory entry */
    int addr = 0;
    for (unsigned int i = 0; i < DIR_PER_BLOCK; i++, addr += sizeof(sde_t)) {
        dentry = (sde_t *)(&buf[addr]);
        if (dentry->name1[0] == DIR_ENTRY_UNUSED) {
            break;
        } else if (dentry->name1[0] == DIR_ENTRY_LAST_AND_UNUSED) {
            /* todo: extend directory */
            break;
        }
    }
    if (addr) {
        kprintf("_alloc_entry: got available directory entry with offset %d\r\n", addr);
    } else {
        kprintf("_alloc_entry: no available entry\r\n");
    }
    return (addr) ? addr : -1;
}




/* read file content and load them into vfs, return the read bytes
 * 
 * @buf:  the storage of data read from sd card
 * @data: the target file
 * @fpos: the current position of file read
 * @len:  the size of user buffer
 */
static int _fat32_read(void *buf, fat32_node_t *node, uint64 fpos, uint64 len) 
{
    fat32_block_t *block;
    uint32 bid, cid, tbid;
    list_node_t *head, *cur;
    uint64 result; /* the actual read byte in this request */
    uint64 bufoff;
    uint64 rsize;
    uint64 fsize = node->size;
    block = NULL;
    tbid = (fpos + len) >> 9;
    bid = fpos >> 9;
    result = 0;
    cid = node->cid;

    /* locate the target data block */
    if (_seek_cache(node, tbid, &block) < 0) {
        /* target block is not in memory, so get data block from disk */
        kprintf("_fat32_read: the target block is not in memory, get it from fat\r\n");
        // if (block) {
        //     cid = get_next_cid(data->fat, block->cid);
        // } else {
        //     cid = data->cid;
        // };
        if (_load_fat32(node, tbid, cid, &block) < 0) {
            kprintf("_fat32_read: [ERROR] target block not in disk\r\n");
            return -1;
        }
    };
    
    head = &node->list;
    list_for_each(cur, head) {
        block = list_entry(cur, fat32_block_t, list);
        if (block->bid == bid) break;
    };
    len = len > fsize ? fsize : len; 
    while (len) {
        bufoff = ((fpos + result) & 0b11111111); /* bufoff should be 0 in the first time */
        rsize = BLOCK_SIZE > len ? len : BLOCK_SIZE;
        rsize = fsize - fpos < rsize ? fsize - fpos : rsize;
        kprintf("fpos: %d\r\n", fpos);
        kprintf("result: %d\r\n", result);
        kprintf("bufoff: %d\r\n", bufoff);
        kprintf("rsize: %d\r\n", rsize);
        kprintf("len: %d\r\n", len);
        if (_read_cache(block, buf, bufoff, rsize) < 0) {
            kprintf("_fat32_read: [ERROR] _read_cache return -1\r\n");
            break;
        }
        block = list_entry(block->list.next, fat32_block_t, list);
        bufoff += rsize;
        result += rsize;
        len -= rsize;
    }
    return result;
}

/* create a new fat32 node and return the pointer to the new node
 * note that the data of this node is still in disk
 * 
 * @type:   the file type
 * @cid:    the cluster id of new node
 * @size:   the file size of new node
 * @pos:    the on-disk offset of directory entry
 */
fat32_node_t *_create_fat_node(uint32 type, uint32 cid, uint32 size, uint32 pos)
{  
    fat32_node_t *node = kmalloc(sizeof(fat32_node_t));
    node->cid = cid;
    node->type = type;
    node->i_pos = pos;
    node->size = size;
    list_init(&node->list);
    kprintf("_create_fat_node: create a node with cid %d\r\n", node->cid);
    return node;
}

int _get_sde_fname(sde_t *dentry, char *pname) {
    /* parse filename part 1 */
    for (int len = 0; len < 8; len++) {
        if (dentry->name1[len] == 0x20) /* 0x20: space */
            break;
        *pname = dentry->name1[len];
        pname++;
    };
    if (dentry->name2[0] != 0x20) {
        *pname++ = '.';
        for (int len = 0; len < 3; len++) {
            if (dentry->name2[len] == 0x20) break;
            *pname = dentry->name2[len];
            pname++;
        }
    }

    *pname = '\0';
    return 1;
}


/* traverse each entry in directory table from sd card and return the dentry 
 * if buflba is set, set buflba to the lba of dentry 
 */
int __lookup_fat32(fat32_node_t *dir_node, sde_t **target, char *name, uint8 *buf) {
    sde_t *dentry;
    struct filename_t fname;
    uint32 cid;
    int found, dirend;

    cid = dir_node->cid;
    found = -1;
    dirend = 0;

    /* initialize memory */
    memset(&fname, 0, sizeof(struct filename_t));
    int lba;
    unsigned long ent_size = sizeof(sde_t);

    /* in this method, we traverse from root directory */
    while (1) {
        lba = cid2lba(dir_node->cid, FAT32INFO);
        kprintf("__lookup_fat32: read data with lba %d from disk\r\n", lba);
        sd_read_block(lba, buf); /* read directory table block */
        uint8 len;
        /* traverse directory entry */
        uint32 addr = 0;
        for (unsigned int i = 0; i < DIR_PER_BLOCK; ++i) {
            dentry = (sde_t *)(&buf[addr]);
            addr += ent_size;
            if (dentry->name1[0] == DIR_ENTRY_LAST_AND_UNUSED) {
                dirend = 1;
                //uart_send_string("__lookup_fat32: no directory entry left\r\n");
                break;
            }
            
            if ((dentry->attr & ATTR_LFN) == ATTR_LFN) { /* dentry is long filename entry */
                // lde_t *ldir;
                // int n;
                // lfn = 1;
                // ldir = (lde_t *)dentry;
                // n = (dentry->name1[0] & 0x3f) - 1;

                // for (int i = 0; (ldir->name1[i] != 0xff) && (i < 10); i += 2) {
                //     fname.part[n].name[i / 2] = ldir->name1[i];
                // }
                // for (int i = 0; (ldir->name2[i] != 0xff) && (i < 12); i += 2) {
                //     fname.part[n].name[5 + i / 2] = ldir->name2[i];
                // }
                // for (int i = 0; (i < 4) && (ldir->name3[i] != 0xff); i += 2) {
                //     fname.part[n].name[11 + i / 2] = ldir->name3[i];
                // }
                continue;
            } else {
                len = 8;
                char *pname = (char *)&fname.fullname[0];
                /* parse filename part 1 */
                for (; len; len--) { 
                    if (dentry->name1[len - 1] != 0x20) /* 0x20: space */
                        break;
                };
                memcopy(pname, (void *)dentry->name1, len);
                pname += len;
                len = 3;
                for (; len; len--) {  /* 0x20: space */
                    if (dentry->name2[len - 1] != 0x20) break;
                }
                if (len > 0) {
                    memcopy(pname++, ".", 1);
                    memcopy(pname, (void *)dentry->name2, len);
                    *(pname + len) = '\0';
                } else {
                    *pname = '\0';
                }
            }  
            if (strmatch(name, (void *)fname.fullname)) {
                kprintf("__lookup_fat32: got entry at addr %d\r\n", addr);
                *target = dentry;
                found = addr;
                break;
            }
            memset(&fname, 0, sizeof(struct filename_t));
        }
        if (found != -1 || dirend)
            break;
        cid = get_next_cid(cid);
        if (invalid_cid(cid)) break;
    }
    if (found == -1) {
        kprintf("__lookup_fat32: file not found\r\n");
    }
    return found;
}

/* find the node with 'name' under 'dir_node' from disk
 * On success, set 'target' to it and return 1
 * On failure, do nothing and return -1
 */
fat32_node_t *_lookup_fat32(fat32_node_t *dir_node, char *name) 
{
    fat32_node_t *node;
    sde_t *dentry;
    int pos;
    uint32 type, cid;
    uint8 buf[BLOCK_SIZE];

    pos = __lookup_fat32(dir_node, &dentry, name, buf);
    if (pos == -1) {
        uart_send_string("_lookup_fat32: file not found\r\n");
        return 0;
    }

    // if (!(dentry->attr & ATTR_FILE_DIR_MASK)) {
    //     uart_send_string("_lookup_fat32: [ERROR] file is not file or directory\r\n");
    //     while(1){};
    // }

    /* the cid should -2 when translating to lba */
    cid = (dentry->highbyte << 16) | dentry->lowbyte; 
    kprintf("_lookup_fat32: cid %d\r\n", cid);
    /* since the file is in disk not in memory, so we create a node for this file */
    type = (dentry->attr & ATTR_ARCHIVE) ? FAT32_TYPE_FILE : FAT32_TYPE_DIR;
    node = _create_fat_node(type, cid, dentry->size, (uint32) pos);
    return node;
}

/* write data into data block (in memory) and return the actucal write size
 * 
 * @data:    data node
 * @block:   data block
 * @buf:     data buffer
 * @blkoff:  offset within block
 * @size:    total size
 */
int _write_cache(fat32_block_t *block, uint8 *buf, 
                 uint32 bufoff, uint32 wsize)
{
    memcopy((void *)&block->buf[bufoff], (void *)&buf[0], wsize);
    block->dirty = 1;
    return 1;
}

/*
 * @ent_off: the offset of entry within block in directory
 */ 
int _sync_create(fat32_node_t *dir_node, fat32_node_t *new_node, char *fname, uint32 ent_off) {
    uart_send_string("in _sync_create\r\n");
    uint32 lba;
    uint8 buf[512];
    
    sde_t *fe = kmalloc(sizeof(sde_t)); /* file entry */
    memset(fe->name1, 0x20, 8);
    memset(fe->name2, 0x20, 3);
    for(int i = 0; *fname != '.'; i++, fname++) {
        fe->name1[i] = *fname;
    };
    fname++;
    for(int i = 0; *fname; i++, fname++) {
        fe->name2[i] = *fname;
    }

    fe->attr = ATTR_ARCHIVE;
    fe->lowbyte = (new_node->cid & 0x0000FFFF);
    fe->highbyte = (new_node->cid >> 16);
    fe->size = new_node->size;
    /* TODO: deal with value */
    fe->ntres = 0x00;
    fe->crttimetenth = 0x00;
    fe->crttime = 0x00;
    fe->crtdate = 0x00;
    fe->lstaccdate = 0x00;
    fe->wrttime = 0x00;
    fe->wrtdate = 0x00;
    fe->crttime = 0x00;
 

    /* add new entry under directory */
    lba = cid2lba(dir_node->cid, FAT32INFO);
    sd_read_block(lba, buf);
    memcopy((void *)&buf[ent_off],(void *)fe, sizeof(sde_t));
    kprintf("_sync_create: write new entry with at %d with offset %d\r\n", lba, ent_off);
    sd_write_block(lba, buf);
    
    /* update cluster status in fat */
    uint32 blk_id = new_node->cid / FAT32INFO->bs.fat32_size;
    uint32 eid = new_node->cid % NUM_ENTRY_FAT; /* entry id */
    lba = FAT32INFO->fat_lba + blk_id;
    uint32 addr = eid2addr(eid);
    sd_read_block(lba, buf);
    memcopy((void *)&buf[addr], (void *)FAT_ENTRY_END, 4);
    sd_write_block(lba, buf);
    kprintf("_sync_create: update cluster status with eid %d in FAT (lba: %d)\r\n", eid, lba);
    return 1;
}

int _sync_write(fat32_node_t *parent, fat32_node_t *node) {
    if (fat32_isdir(node)) {
        kprintf("file is directory\r\n");
        return -1;
    };
    fat32_block_t *block;
    list_node_t *head;
    // uint32 cid;
    // uint64 blkoff;
    uint32 lba;
    lba = cid2lba(node->cid, FAT32INFO);
    head = &node->list;
    block = list_first_entry(head, fat32_block_t, list);
    while(1) {
        // blkid = block->bid;
        // lba += blkid;
        kprintf("_sync_write: write data block at lba %d\r\n", lba);
        sd_write_block(lba, (void *)block->buf);
        block->dirty = 0;
        if (block->list.next == head)
            break;
    };

    /* update file metadata */
    uint8 buf[BLOCK_SIZE];
    uint32 offset = node->i_pos;
    lba = cid2lba(parent->cid, FAT32INFO);
    sd_read_block(lba, buf);
    sde_t *ent = (sde_t *)&buf[offset];
    ent->size = node->size;
    sd_write_block(lba, buf);
    return 1;
}

/* append data into file
 */
// int _fat32_append(void *buf, fat32_node_t *data, uint64 fpos, uint64 len) {
// return 1;
// }

/* write data into file (in memory), and return the write byte 
 * return -1 if error
 * 
 * @buf:     write buffer
 * @data:    the file node
 * @fpos:    the position (offset) of file write
 * @len:     the total size of write data
 */
int _fat32_write(file_t *file, fat32_node_t *inode, void *buf, uint64 fpos, 
                 uint64 len) 
{
    fat32_block_t *block;
    uint32 bid; /* current block id */
    uint64 bufoff, result, wsize;
    list_node_t *head, *cur;
    int ret;
    head = cur = &inode->list;
    block = NULL;
    result = 0;
    bufoff = 0; /* offset within block */
    bid = 0;
    /* TODO: fix alloc_clutser bug */
    while (len) {
        if (cur->next == head) {
            ret = _alloc_free_cluster();
            if (ret < 0) {
                kprintf("_fat32_write: no available space in disk\r\n");
                break;
            }
            MARK_BLOCK_USED(block->cid, ret);

            block = kmalloc(sizeof(fat32_block_t));
            block->bid = bid++;
            block->cid = (uint32) ret;
            list_add_tail(&block->list, head);
        } else {
            block = list_entry(cur->next, fat32_block_t, list);
            
        }
        cur = cur->next;
        memset(block->buf, 0, 512);
        
        wsize = len > BLOCK_SIZE ? BLOCK_SIZE : len;
        kprintf("_fat32_write: trying write %d bytes data into block with id %d\r\n", wsize, block->bid);
        if (_write_cache(block, buf, bufoff, wsize) < 0) {
            kprintf("_fat32_write: [ERROR] _write_cache return -1\r\n");
            ret = -1;
            break;
        }
        bufoff += wsize;
        result += wsize;
        len -= wsize;
    }
    return result;
}



/* read boot sector from disk
 * reference: https://elixir.bootlin.com/u-boot/latest/source/fs/fat/fat.c#L492
 */
int _load_boot_sector(boot_sector_t *bs, void *buf, int lba) {
    sd_read_block(lba, buf);
    /* todo: handle read error */
    memcopy((void *)bs, (void *)buf, sizeof(boot_sector_t));
    // bs->sector_size = FAT2CPU16(bs->sector_size);
    // bs->dir_entries = FAT2CPU16(bs->dir_entries);
    // if (bs->sector_per_fat16 == 0) {
    //     bs->fat32_size = FAT2CPU32(bs->fat32_size);
    //     bs->extflags = FAT2CPU16(bs->extflags);
    //     bs->root_cluster = FAT2CPU32(bs->root_cluster);
    //     bs->info_sect = FAT2CPU16(bs->info_sect);
    //     bs->backup_boot = FAT2CPU16(bs->backup_boot);
    // };
    kprintf("fat32: Loading boot sector...\r\n");
    kprintf("\tsector size: %d bytes\r\n", bs->sector_size);
    kprintf("\tnumber of sectors per cluster: %d\r\n", bs->cluster_size);
    kprintf("\tnumber of clusters per fat tables: %d\r\n", bs->fat32_size);
    kprintf("\tnumber of fat tables: %d\r\n", bs->fats);
    kprintf("\treserved sectors: %d\r\n", bs->reserved);
    kprintf("\troot directory: %d\r\n", bs->root_cluster);
    return 1;
}

int _sync_fat_table() {
    uint32 lba, num_blks;
    uint8 buf[BLOCK_SIZE];
    
    void *table = FAT32INFO->table;
    boot_sector_t bs = FAT32INFO->bs;
    num_blks = bs.cluster_size * bs.fat32_size * bs.fats; /* 2018 */
    lba = FAT32INFO->fat_lba;
    for (uint32 i = 0; i < num_blks; i++) {
        memcopy((void *)buf, table, BLOCK_SIZE);
        sd_write_block(lba, buf);
        table += 64;
        lba++;
    }
    return 1;
}

int _setup_fat_table(fat32_info_t *fat) {
    uint32 num_blks, size, idx, lba, eid;
    fat32_centry_t buf[DIR_PER_BLOCK];
    boot_sector_t bs = fat->bs;
    num_blks = bs.cluster_size * bs.fat32_size * bs.fats;
    size = num_blks * 4;
    fat32_centry_t *table = kmalloc(size);
    kprintf("\tFAT table entries: %d\r\n", num_blks);
    kprintf("\tFAT table size: %d bytes\r\n", size);
    memset(table, 0, size);
    lba = fat->fat_lba;
    eid = 0;
    for(uint32 i = 0; i < num_blks; i++, lba++) { 
        sd_read_block(lba, (void *)buf);
        for (idx = 0; idx < DIR_PER_BLOCK; idx++, eid++) {
            table[eid] = buf[idx];
        };
    }
    fat->table = table;
    return 1;
};


int _load_directory(fat32_info_t *fat, fat32_node_t *dir_node) {
    uint8 buf[BLOCK_SIZE];
    char fname[16];
    uint32 lba;
    uint32 type, cid, size, bufoff;
    sde_t *dentry;
    lba = cid2lba(dir_node->cid, fat);
    fat32_node_t *node;
    vnode_t *vfs_node, *parent;
    parent = dir_node->vfs_node;
    /* load dentry in root directory */
    kprintf("lba: %d\r\n", lba);
    sd_read_block(lba, buf);
    bufoff = 0;
    while(1) {
        dentry = (sde_t *)(&buf[bufoff]);
        if (dentry->name1[0] == DIR_ENTRY_LAST_AND_UNUSED)
            break;
        if((dentry->attr & ATTR_LFN) == ATTR_LFN) {
            // kprintf("lfn\r\n");
            bufoff += sizeof(sde_t);
            continue;
        }

        type = (dentry->attr & ATTR_ARCHIVE) ? FAT32_TYPE_FILE : FAT32_TYPE_DIR;
        cid = (dentry->highbyte << 16) | dentry->lowbyte;
        size = dentry->size;
        node = _create_fat_node(type, cid, size, bufoff);
        
        _get_sde_fname(dentry, fname);
        kprintf("fname: %s\r\n", fname);
        kprintf("type: %d\r\n", type);
        vfs_node = _create_vnode(parent, node, fname);
        list_add_tail(&vfs_node->d_child, &parent->d_subdirs);
        // if (type == FAT32_TYPE_DIR && fname[0] != '.')
        //     _load_directory(fat, node);
    
        bufoff += sizeof(sde_t);
    }
    return 1;
}

int fat32_mount(fs_t *fs, struct mount *mount) {
    fat32_node_t *new_root; /* new_root of fat32 */
    fat32_mount_t *newmount;
    uint32 lba = 0;
    int ret;
    uint8 buf[BLOCK_SIZE];
    
    sd_read_block(0, buf); 
     if (buf[510] != 0x55 || buf[511] != 0xaa) {
        uart_send_string("[Error] the end label is not 0x55AA\r\n");
        return -1;
    }
    // pentry_t *partition = (pentry_t *)&buf[446];
    // if (partition[0].type != 0xb && partition[0].type != 0xc)
    //     return -1;
    // lba = partition->lba;

    FAT32INFO = kmalloc(sizeof(fat32_info_t));
    ret = _load_boot_sector(&FAT32INFO->bs, buf, lba);
    if (ret != 1) {
        uart_send_string("load boot sector failed\r\n");
        while(1){};
    };

    FAT32INFO->fat_lba = lba + FAT32INFO->bs.reserved;
    FAT32INFO->data_lba = FAT32INFO->fat_lba +
                       FAT32INFO->bs.fats * FAT32INFO->bs.fat32_size;

    /* load FS Info 
     * reference: https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#FS_Information_Sector
     */
    sd_read_block(1, buf);
    memcopy((void *)&FAT32INFO->free_sectors, (void *)&buf[488], 4);
    memcopy((void *)&FAT32INFO->next_free, (void *)&buf[492], 4);
    FAT32INFO->next_free = FAT32INFO->next_free + FAT32INFO->data_lba - 2;

    _setup_fat_table(FAT32INFO);
    new_root = kmalloc(sizeof(fat32_node_t));
    new_root->i_pos = 0;
    new_root->cid = 2;
    list_init(&new_root->list);
    new_root->type = FAT32_TYPE_DIR;

    vnode_t *mnt_point = mount->root;
    vnode_t *new_node = kmalloc(sizeof(vnode_t));

    strcopy(new_node->name, mnt_point->name);
    new_node->mount = 0;
    new_node->parent = mnt_point->parent;
    new_node->f_ops = &FAT32_FOPS;
    new_node->v_ops = &FAT32_VOPS;
    new_node->internal = new_root;
    list_init(&new_node->d_child);
    list_init(&new_node->d_subdirs);

    new_root->vfs_node = new_node;
    _load_directory(FAT32INFO, new_root);
    mount->root = new_node;

    // preempt_disable();
    newmount = kmalloc(sizeof(fat32_mount_t));
    newmount->mount = mount;
    fs->root = new_node;
    list_add_tail(&newmount->list, &FAT32_MOUNT_LIST);
    // preempt_enable();
    kprintf("fat32_mount: mount fat32 at '%s'\r\n", mount->root->name);
    return 0;
}

/* open file in FAT32 and return file descriptor */
int fat32_open(vnode_t *file_node, struct file **target) {
    file_t *t = *target;
    t->vnode = file_node;
    t->f_pos = 0;
    t->f_ops = file_node->f_ops;
    return 0;
};


/* FAT32 read API */
int fat32_read(file_t *file, void *buf, unsigned int len) {
    uart_send_string("in fat32_read\r\n");
    
    fat32_node_t *node = file->vnode->internal;
    if (fat32_isdir(node)) {
        uart_send_string("fat32_read: file is directory\r\n");
        return -1;
    }
    if (file->f_pos >= node->size) {
        kprintf("fat32_read: reach to EOF\r\n");
        kprintf("fat32_read: file->f_pos %d\r\n", file->f_pos);
        kprintf("fat32_read: node->size %d\r\n", node->size);
        return 0;
    }
    int ret = _fat32_read(buf, node, file->f_pos, len);
    if (ret > 0)
        file->f_pos += ret;

    return ret;
}

/* FAT32 wirte API */
int fat32_write(file_t *file, void *buf, unsigned int len) {
    // if (!len) return len;
    int ret;
    fat32_node_t *node = file->vnode->internal;
    if (fat32_isdir(node)) 
        return -1;
    ret = _fat32_write(file, node, buf, file->f_pos, len);
    if (ret < 0) {
        uart_send_string("fat32_write: error in _fat32_write\r\n");
        return -1;
    };
    file->f_pos += ret;
    node->size += ret;
    vnode_t *dir_node = file->vnode->parent;
    _sync_write((fat32_node_t *)dir_node->internal, node);
    kprintf("fat32_write: write %d byte into file\r\n", ret);
    return ret;
}


/* FAT32 lookup API 
 * search file on disk
 */
int fat32_lookup(vnode_t *vfs_node, vnode_t **target, char *fname) {
    fat32_node_t *dir_node = vfs_node->internal;
    kprintf("fat32_lookup: trying to find '%s' under '%s'\r\n", fname, vfs_node->name);

    if (dir_node->type != FAT32_TYPE_DIR) {
        uart_send_string("fat32_lookup: dir_node is not directory\r\n");
        return -1;
    }
    // if (_lookup_cache(dir_node, target, fname) == 1)
    //     return 1;
    fat32_node_t *node = _lookup_fat32(dir_node, fname);
    if (!node) {
        kprintf("fat32_lookup: file '%s' is not in disk\r\n", fname);
        return -1;
    }
    
    /* link vnode with new node */
    vnode_t *new = _create_vnode(vfs_node, node, fname);
    *target = new;

    return 1;
}


/* FAT32 create API
 * create a new file under dir_node 
 */
int fat32_create(vnode_t *vfs_node, vnode_t **target, char *name) {
    fat32_node_t *dir_node = vfs_node->internal;

    if (dir_node->type != FAT32_TYPE_DIR) {
        uart_send_string("fat32_create: vfs_node is not directory\r\n");
        return -1;
    }
    if (fat32_lookup(vfs_node, target, name) == 1) {
        kprintf("fat32_create: file '%s' already exists\r\n", name);
        return -1;
    }
    
    /* allocate a new entry for this file */
    int ent_off = _alloc_entry(dir_node);
    if (ent_off < 0) {
        kprintf("fat32_create: _alloc_entry failed\r\n");
        return -1;
    }
    /* allocate free cluster from disk for data block */
    int cid = _alloc_free_cluster();
    MARK_BLOCK_END(cid);
    if (cid < 0) {
        kprintf("fat32_create: _alloc_free_cluster failed\r\n");
        return -1;  
    }
    fat32_node_t *node = _create_fat_node(FAT32_TYPE_FILE, cid, 0, (uint32) ent_off);
    vnode_t *new = _create_vnode(vfs_node, node, name);
    *target = new;
    kprintf("fat32_create: create a file '%s' under directory '%s'\r\n", name, vfs_node->name);
    _sync_create(dir_node, node, name, (uint32) ent_off);
    return 1;
}

int fat32_mkdir(vnode_t *vfs_node, vnode_t **target, char *name) {
    fat32_node_t *dir_node = vfs_node->internal;
    if (dir_node->type != FAT32_TYPE_DIR)
        return -1;

    if (fat32_lookup(vfs_node, target, name) == 1) {
        return -1;
    }
    
    /* allocate a new entry for this file */
    int ent_off = _alloc_entry(dir_node);
    if (ent_off < 0) 
        return -1;

    /* allocate free cluster from disk for data block */
    int cid = _alloc_free_cluster();
    MARK_BLOCK_END(cid);
    if (cid < 0) {
        kprintf("fat32_create: _alloc_free_cluster failed\r\n");
        return -1;  
    }
    fat32_node_t *node = _create_fat_node(FAT32_TYPE_DIR, cid, 0, (uint32) ent_off);
    vnode_t *new = _create_vnode(vfs_node, node, name);
    *target = new;
    kprintf("fat32_create: create a file '%s' under directory '%s'\r\n", name, vfs_node->name);
    _sync_create(dir_node, node, name, (uint32) ent_off);
    return 1;
}

int fat32_close(file_t *fd) {
    fd->vnode = 0;
    fd->f_pos = 0;
    fd->f_ops = 0;
    return 1;
}

struct filesystem *fat32_init(void) {
    return &FAT32FS;
};

