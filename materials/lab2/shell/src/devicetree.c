#include "uart.h"
#include "devicetree.h"
#include "utils.h"

/* load 32 bit integer data store in big-endian order */
unsigned int load_uint32be(void *ptr)
{
    /*
     * e.g. 0000 0101 1111 1010
     *   -> 1111 1010 0000 0101
     *   -> 1010 1111 0101 0000
    */
    unsigned int x = *(unsigned int *) ptr;
    x = ((x & 0xffff0000) >> 16) | ((x & 0x0000ffff) << 16);
    x = ((x & 0xff00ff00) >>  8) | ((x & 0x00ff00ff) <<  8);
    return x;
}

int node_match(char *ptr, char *name)
{
    if (!*ptr) return 0; /* root node */
    for (; *ptr != '@'; ptr++, name++)
        if (*ptr != *name) return 0;
    return 1;
}


/* parse_node: parse node to get property information 
 * pnode: pointer to node entry
 * strblock: pointer to strings block
 */
void parse_node(char *pnode, char *strblock)
{   
    unsigned int token, tmp, len, nameoff;
    char *ptr = pnode;

    while (*ptr != '@')
        ptr++;
    ptr++;
    uart_send_string("address:0x");
    while (*ptr)
        uart_send_char(*ptr++);
    uart_send_char('\n');

    tmp = strlen(pnode) + 1;
    alignto(&tmp, 4);
    ptr = pnode + tmp;
    token = load_uint32be(ptr);
    while (token != FDT_END_NODE) {
        ptr += 4;
        switch (token) {
            case FDT_PROP:
                len = load_uint32be(ptr);
                ptr += 4;
                nameoff = load_uint32be(ptr);
                ptr += 4;
                parse_prop(ptr, strblock, len, nameoff);
                alignto(&len, 4);
                ptr += len;
                break;
            case FDT_NOP:
                break;
            case FDT_END_NODE:
                break;
            case FDT_BEGIN_NODE:
                tmp = strlen(ptr) + 1;
                alignto(&tmp, 4);
                ptr += tmp;
                break;
            default:
                uart_send_string("Error: unknown token");
                break;
        }
        token = load_uint32be(ptr);
    }
    return;
}

/* TODO: finish the value parser */
// void parse_prop_val(char *ptr)
// {
//     /* all value types 
//      * empty 
//      * u32
//      * u64
//      * string
//      * prop-encoded-array
//      * phandle
//      * stringlist
//      */
// }



/* parse_prop: parse node property
 * ptr: pointer to node property
 * strblock: pointer to strings block
 * len: the length of property value
 * nameoff: an offset from string block to property name.
*/
void parse_prop(char *ptr, char *strblock, unsigned int len, unsigned int nameoff)
{
    char *name = strblock + nameoff;
    uart_send_string(name);
    uart_send_string(": '");
    while(len-- > 0)
        uart_send_char(*ptr++);
    uart_send_string("'\n");
}



/* traverse_dtb: search device information from struct block
 * device: the device name.
*/
int traverse_dtb(char *device)
{
    /* The while dtb file structure
	+-----------------+ <- BASE_DTB
	| fdt_header      | 
    +-----------------+
	| free space      |
	+-----------------+
	| reserved memory |
	+-----------------+
	| free space      |
	+-----------------+ <- ptr
	| structure block | structure block contains all node data of device tree
	+-----------------+
	| free space      |
	+-----------------+ <- strblock
	| strings block   | strings block contains the property information
	+-----------------+
	*/

    unsigned int token, tmp, len;
    char *ptr, *strblock;
    struct dtb_header *header;

    header = (struct dtb_header *) BASE_DTB;

    ptr = (char *)BASE_DTB + load_uint32be(&header->off_dt_struct);
    strblock = (char *)BASE_DTB + load_uint32be(&header->off_dt_strings);

    token = load_uint32be(ptr);
    while (token != FDT_END) {
        ptr += 4;
        switch (token) {
            case FDT_BEGIN_NODE:
                if (node_match(ptr, device)) {
                    parse_node(ptr, strblock);
                    return 1;
                };
                tmp = strlen(ptr) + 1;
                alignto(&tmp, 4);
                ptr += tmp;
                break;
            case FDT_END_NODE:
                break;
            case FDT_PROP:
                len = load_uint32be((char *)ptr);
                alignto(&len, 4);
                ptr += (len + 8);
                break;
            case FDT_NOP:
                break;
            case FDT_END:
                break;
            default:
                uart_send_string("Error: unknown token\n");
                return 0;
        }
        token = load_uint32be(ptr);
    };
    return 1;
}
