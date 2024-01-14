#include "vfs.h"
#include "list.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

#define BLOCK_SIZE 512
/* little-endian to big-endian helper function */
#define FAT2CPU16(x) ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8))
#define FAT2CPU32(x) ((((x) & 0x000000ff) << 24) |	\
			         (((x) & 0x0000ff00) << 8)  |	\
			         (((x) & 0x00ff0000) >> 8)  |	\
			         (((x) & 0xff000000) >> 24))

/* partition entry in MBR */
typedef struct partition_entry {
    uint8 status;          /* 0x80 for active, 0x00 for inactive */
    uint8 chss_head;
    uint8 chss_sector;
    uint8 chss_cylinder;
    uint8 type;            /* partition type. 0xc for FAT32 with LBA */
    uint8 chse_head;       /* end location of this partition */
    uint8 chse_sector;
    uint8 chse_cylinder;
    uint32 lba;            /* address of boot sector (the start of FAT32) */
    uint32 sectors;        /* number of sector in this partition */
} pentry_t;

/* boot sector information 
 * This information is located in the first sector of a FAT32 file system
 * which is incidated in partition entry
 * reference: https://github.com/u-boot/u-boot/blob/master/include/fat.h#L93
 */
typedef struct fat32_boot_sector {
    uint8 jmpboot[3];              /* jump code */
    char odmname[8];               /* ODM name */
    uint16 sector_size;            /* sector size (in byte) */
    uint8 cluster_size;            /* number of sectors per cluster */
    uint16 reserved;               /* number of sectors in boot sector (offset: 0x0E) */
    uint8 fats;                    /* number of FATs in this partition */
    uint16 dir_entries;            /* number of root directory entries */
    uint16 old_sector_cnt;         /* number of sectors */
    uint8 media;                   /* media code */
    uint16 sector_per_fat16;       /* number of sectors in a FAT16 would be 0 if FAT32 */
    uint16 sector_per_track;       /* number of sectors in a track */
    uint16 heads;                  /* number of heads in this partition */
    uint32 hiddens;                /* number of hidden sectors */
    uint32 total_sect;             /* number of sectors in partition (offset: 0x20) */
    uint32 fat32_size;             /* number of sectors(entries) in a FAT (offset: 0x24) */
    uint16 extflags;               /* bit 8: fat mirroring, low 4: active fat */        
    uint16 version;                /* version of FAT32 drive */
    uint32 root_cluster;           /* first cluster in root directory */
    uint16 info_sect;              /* file system info sector */  
    uint16 backup_boot;            /* backup boot sector */
    uint8 reserved2[6];            /* unused */
} __attribute__((__packed__)) boot_sector_t;

/* the entry(cluster) of FAT
 * note that cluster is composed of several contiguous blocks  
 */
typedef struct fat32_cluster_entry {
    union {
        uint32 val;
        struct {
            uint32 idx: 28;
            uint32 reserved: 4;
        };
    };
}  __attribute__((__packed__)) fat32_centry_t;


/* the data block of a file */
typedef struct fat32_file_block {
    list_node_t list;       /* list of data block of the file */
    uint32 bid;             /* id of data block within block list (in memory) */
    uint32 cid;             /* the cluster id of FAT entry (on disk) */  
    uint32 dirty;           /* indiate it should be write back to disk */
    uint8 buf[BLOCK_SIZE];  /* data block loaded from disk */
} fat32_block_t;

/* FATInfo global data */
typedef struct fat32_info {
    boot_sector_t bs;    /* boot sector */
    uint32 fat_lba;      /* the start of FAT region */
    uint32 data_lba;     /* the start of data region (root directory) */
    void *table;
    uint32 free_sectors; /* unused */
    uint32 next_free;    /* unused */
} fat32_info_t;

typedef struct fat32_inode {
    // char *name;         /* for now we store filename as string */
    vnode_t *vfs_node;
    list_node_t list;   /* list of data block */
    uint32 cid;         /* the first logical block address */
    uint32 i_pos;       /* on-disk position(index) of directory entry */
    uint32 type;
    uint32 size;
} fat32_node_t;

typedef struct fat32_mount_t {
    list_node_t list; /* link fat_mount_t */
    struct mount *mount;
} fat32_mount_t;

/* short filename directory entry */
typedef struct short_dentry_t {
    uint8 name1[8];      /* file name */
    uint8 name2[3];      /* file name */
    uint8 attr;          /* file attribute */
    uint8 ntres;         /* reserved */
    uint8 crttimetenth;  /* creation time of ms */
    uint16 crttime;      /* creation time */
    uint16 crtdate;      /* creation time */
    uint16 lstaccdate;   /* last access date */
    uint16 highbyte;     /* high-order bytes of first cluster address */
    uint16 wrttime;      /* write time */
    uint16 wrtdate;      /* write date */
    uint16 lowbyte;      /* low-order bytes of the first cluster address */
    uint32 size;         /* file size */
} __attribute__((__packed__)) sde_t;

/* long filename directory entry */
typedef struct long_dentry_t {
    uint8 order;
    uint8 name1[10];
    uint8 attr;
    uint8 type;
    uint8 checksum;
    uint8 name2[12];
    uint16 fstcluslo;
    uint8 name3[4];
} __attribute__((__packed__)) lde_t;

struct filename_t {
    union {
        uint8 fullname[256];
        struct {
            uint8 name[13];
        } part[20];
    };
};

struct filesystem *fat32_init(void);
int fat32_mount(struct filesystem *fs, struct mount *mount);
int fat32_open(vnode_t *file_node, file_t **target);
int fat32_read(file_t *file, void *buf, unsigned int len);
int fat32_lookup(vnode_t *dir_node, vnode_t **target, char *fname);
int fat32_create(vnode_t *dir_node, vnode_t **target, char *name);
int fat32_write(file_t *file, void *buf, unsigned int len);
int fat32_close(file_t *fd);
int fat32_mkdir(vnode_t *vfs_node, vnode_t **target, char *name);
