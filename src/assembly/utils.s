.globl memzero
memzero:
	str xzr, [x0], #8
	subs x1, x1, #8
	b.gt memzero
	ret
    

.globl memcopy        // copy memory content from x0 to x1 with size x2
memcopy:
    b memcopy_cmp
memcopy_start:  
    ldrb w3, [x1], #1
    strb w3, [x0], #1
    subs x2, x2, #1
memcopy_cmp:
    cmp x2, 1
    bge memcopy_start
memcopy_finish:
    ret

.globl delay
delay:
    subs x0, x0, #1
    bne delay
    ret

.globl put32
put32:
    str w1, [x0]
    ret

.globl get32
get32:
    ldr w0, [x0]
    ret

.globl memset
memset:
    b memset_1_cmp
memset_1:
    strb w1, [x0], #1
    subs x2, x2, #1
memset_1_cmp:
    cmp x2, 1
    bge memset_1
memset_ok:
    ret
