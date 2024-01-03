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
    bl      from_el2_to_el1
    mov     sp, x1            // stack grows to a lower address per AAPCS64
                        
    adr     x0, evtable       // setup exception vector table
    msr     vbar_el1, x0

    bl		core_timer_enable // enable timer interrupt
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



// exception vector table
.align 11                     // vector table should be aligned to 0x800
.global evtable
evtable:                      
    b exception_handler       // branch to a handler function.
    .align 7                  // entry size is 0x80, .align will pad 0
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b exception_handler1      // uart ex interrupt and timer interrupt
    .align 7
    b exception_handler1      // interrupt from the current EL (e.g timer)
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b lower_sync_eh     // synchronous interrupt from lower EL in aarh64 
    .align 7
    b exception_handler1
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7

    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7
    b exception_handler
    .align 7


// save general registers to stack
.macro save_reg                 
    sub sp, sp, 32 * 8
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]
.endm

// load general registers from stack
.macro load_reg
    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 32 * 8
.endm

// Save content and jump to function `handle_irq` in c
exception_handler1:
    save_reg
    bl handle_irq
    load_reg
    eret

// Save content and jump to function `handle_exception` in c
exception_handler:
    save_reg
    bl handle_exception 
    load_reg
    eret

/* save the state of 'before entering kernel' into kernel stack */
.macro entry_kernel el
    sub sp, sp, 17 * 16
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]

    .if \el == 0
    mrs x0, sp_el\el /* before entering kernel we use sp_el0, so we need to save sp_el0 */
    stp x30, x0, [sp, 16 * 15]
    .else
    str x30, [sp, 16 * 15]
    .endif

    mrs x0, elr_el1
    mrs x1, spsr_el1
    stp x0, x1, [sp, 16 * 16]
.endm

.macro exit_kernel el
    ldp x0, x1, [sp, 16 * 16]
    msr elr_el1, x0
    msr spsr_el1, x1

    .if \el ==0
    ldp x30, x0, [sp, 16 * 15]
    msr sp_el\el, x0
    .else
    ldr x30, [sp, 16 * 15]
    .endif

    ldp x28, x29, [sp ,16 * 14]
    ldp x26, x27, [sp ,16 * 13]
    ldp x24, x25, [sp ,16 * 12]
    ldp x22, x23, [sp ,16 * 11]
    ldp x20, x21, [sp ,16 * 10]
    ldp x18, x19, [sp ,16 * 9]
    ldp x16, x17, [sp ,16 * 8]
    ldp x14, x15, [sp ,16 * 7]
    ldp x12, x13, [sp ,16 * 6]
    ldp x10, x11, [sp ,16 * 5]
    ldp x8, x9, [sp ,16 * 4]
    ldp x6, x7, [sp ,16 * 3]
    ldp x4, x5, [sp ,16 * 2]
    ldp x2, x3, [sp ,16 * 1]
    ldp x0, x1, [sp ,16 * 0]
    add sp, sp, 17 * 16
    eret
.endm

lower_sync_eh:
    entry_kernel 0               // save process context before entrying kernel
    mov x0, sp
    mrs x1, esr_el1
    bl el0_sync_handler

    mov x0, sp
    bl handle_signal      // handle signal
    ldp x0, x1, [sp, 16 * 16]
    msr elr_el1, x0
    msr spsr_el1, x1


    ldp x30, x0, [sp, 16 * 15]
    msr sp_el0, x0


    ldp x28, x29, [sp ,16 * 14]
    ldp x26, x27, [sp ,16 * 13]
    ldp x24, x25, [sp ,16 * 12]
    ldp x22, x23, [sp ,16 * 11]
    ldp x20, x21, [sp ,16 * 10]
    ldp x18, x19, [sp ,16 * 9]
    ldp x16, x17, [sp ,16 * 8]
    ldp x14, x15, [sp ,16 * 7]
    ldp x12, x13, [sp ,16 * 6]
    ldp x10, x11, [sp ,16 * 5]
    ldp x8, x9, [sp ,16 * 4]
    ldp x6, x7, [sp ,16 * 3]
    ldp x4, x5, [sp ,16 * 2]
    ldp x2, x3, [sp ,16 * 1]
    ldp x0, x1, [sp ,16 * 0]
    add sp, sp, 17 * 16
    eret
    // exit_kernel 0                // restore process context


.global ret_from_child_fork
ret_from_child_fork:
    mov x0, sp
    // bl exit_to_user_mode      // handle signal
    exit_kernel 0                // restore process context

core_timer_enable:                  
    mov x0, 1
    msr cntp_ctl_el0, x0            // set cntp_ctl_el0 to 1 to enable cpu timer 
    // mrs x0, cntfrq_el0              // load timer frequency
    // mov x3, 2                       // set expired time       
    // mul x0, x0, x3                  //     cntp_tval_el0 = time * frequency    
    // msr cntp_tval_el0, x0           //     cntp_tval_el0: the elapsed time of invoking a interrupt after booting.
    ldr x1, =0x40000040             // 0x40000040: CORE0_TIMER_IRQ_CTRL
    str w0, [x1]                    // unmask timer interrupt
    ret

from_el2_to_el1:
    mov x0, #(1 << 31)              // make EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x345                   // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    eret                            // return to EL1

.global switch_to
switch_to:                   
    mov x10, 152               // 152 is the offset of callee-saved registers in trapframe_t
    add x11, x0, x10 
    stp x19, x20, [x11, 16 * 0]
    stp x21, x22, [x11, 16 * 1]
    stp x23, x24, [x11, 16 * 2] 
    stp x25, x26, [x11, 16 * 3]
    stp x27, x28, [x11, 16 * 4]
    stp x29, x30, [x11, 16 * 5]
    mov x9, sp
    str x9, [x11, 16 * 6]

    add x12, x1, x10
    ldp x19, x20, [x12, 16 * 0]
    ldp x21, x22, [x12, 16 * 1]
    ldp x23, x24, [x12, 16 * 2]
    ldp x25, x26, [x12, 16 * 3]
    ldp x27, x28, [x12, 16 * 4]
    ldp x29, x30, [x12, 16 * 5]
    ldr x9, [x12, 16 * 6]
    mov sp,  x9
    // msr tpidr_el1, x1
    ret

.global get_current
get_current:
    mrs x0, tpidr_el1
    ret

.globl ret_from_fork
ret_from_fork:
    bl    schedule_tail
    cbz x19, ret_to_user
    mov    x0, x20
    blr    x19         // should never return
ret_to_user:
    // msr elr_el1, x0    // setup exception return address
    // msr sp_el0, x1     // setup user stack
    mov x13, 0          // enable interrupt ({D, A, I, F} = 0 (unmasked)) 
    msr spsr_el1, x13   //     EL0 ({M[3:0]} = 0)
    eret


.section ".data"	          // BASE_DTB is in data section
BASE_DTB: .dc.a 0x0	          // defines 'BASE_DTB' to be a 8-byte constant with a value of 0x0
