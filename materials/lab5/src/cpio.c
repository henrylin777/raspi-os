#include "cpio.h"
#include "uart.h"
#include "utils.h"
#include "malloc.h"

unsigned long HEADER_SIZE = sizeof(struct cpio_header);

unsigned long str2int(const char *str, int nchar) {
    unsigned long num = 0;
    for (int i = 0; i < nchar; i++) {
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
    while (strmatch(pname, "TRAILER!!!") == 0) {
        if ((strmatch(pname, fname) != 0)) {
            *ret = ptr;
            return file_size;
        }
        pheader = (struct cpio_header *)ptr;
        headpath_size = str2int(pheader->c_namesize, (int)sizeof(pheader->c_namesize)) + HEADER_SIZE;
        file_size =  str2int(pheader->c_filesize, (int)sizeof(pheader->c_filesize));
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
        offset = str2int(phead->c_namesize,(int) sizeof(phead->c_namesize)) + HEADER_SIZE;
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
		fname_size = str2int(header->c_namesize, (int)sizeof(header->c_namesize));
		fname_size += HEADER_SIZE;
		file_size = str2int(header->c_filesize, (int)sizeof(header->c_filesize));
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
    offset = str2int(phead->c_namesize,(int) sizeof(phead->c_namesize)) + HEADER_SIZE;
    fsize = str2int(phead->c_filesize,(int) sizeof(phead->c_filesize));

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
