#include "cpio.h"
#include "uart.h"
#include "utils.h"

unsigned long HEADER_SIZE = sizeof(struct cpio_header);

/* get the file object with fname from cpio arvhive and return the pointer to the head of file.
 * return 0 if not found 
*/
char *cpio_find_file(char *fname)
{
    unsigned long headpath_size, file_size;
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
        if ((strmatch(pname, fname) != 0))
            return ptr;
        pheader = (struct cpio_header *)ptr;
        headpath_size = str2int(pheader->c_namesize, (int)sizeof(pheader->c_namesize)) + HEADER_SIZE;
        file_size =  str2int(pheader->c_filesize, (int)sizeof(pheader->c_filesize));
        alignto(&headpath_size, 4); 
        alignto(&file_size, 4);
        ptr += (headpath_size + file_size);
        pname = ptr + HEADER_SIZE;
    };
    return 0;
}



/* print the content of the file (like command `cat`) */
void cpio_cat(char *fname)
{
    unsigned long offset;
    struct cpio_header* phead;
    char *ptr = cpio_find_file(fname);
    if (*ptr) {
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
	    
		alignto(&fname_size, 4);
		alignto(&file_size, 4);

		ptr += (fname_size + file_size);
	}
}
