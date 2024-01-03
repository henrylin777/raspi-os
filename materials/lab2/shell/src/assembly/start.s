.section ".text.boot"
.global _start
.global BASE_DTB	          // define a global variable 'BASE_DTB'

_start:
    ldr     x1, =BASE_DTB     // load the address of `BASE_DTB` into reg `x1`  
    str     x0, [x1]          // store the address of dtb file into x1(the address of`BASE_DTB`)

    mrs     x1, mpidr_el1     // execute only on core0
    and     x1, x1, #3        //     if cpu id == 0, then jump to setting and start to initialize
    cbz     x1, setting       //     otherwise we halting
    
halting:  
    wfe
    b       halting

setting:
    ldr     x1, =_start       // set top of stack just before our code
    mov     sp, x1            // (stack grows to a lower address per AAPCS64)
    ldr     x1, =__bss_start  // prepare variable for clearning bss
    ldr     w2, =__bss_size   // prepare variable for clearning bss

clear_bss:                    // clear bss section for c code
    cbz     w2, shell_main    // jump to label 'main' if bss clean-up is finished
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, clear_bss

shell_main:  
    bl      main              // jump to function 'main' in c code, it should not return
    b       halting           // jump to halting if failure

.section ".data"	          // BASE_DTB is in data section
BASE_DTB: .dc.a 0x0	          // defines 'BASE_DTB' to be a 8-byte constant with a value of 0x0
