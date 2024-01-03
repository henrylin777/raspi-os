
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

.global switch_to
switch_to:                     // x0: stack pointer of current thread, 16 * 8 = 128 bits -> 2 regsiters 
    mov x10, 152               // 152 is the offset of callee-saved registers in trapframe_t
    add x11, x0, x10 
    stp x19, x20, [x11, 16 * 0]
    stp x21, x22, [x11, 16 * 1]
    stp x23, x24, [x11, 16 * 2] 
    stp x25, x26, [x11, 16 * 3]
    stp x27, x28, [x11, 16 * 4]
    stp fp, lr, [x11, 16 * 5]
    mov x9, sp
    str x9, [x11, 16 * 6]

    add x12, x1, x10
    ldp x19, x20, [x12, 16 * 0]
    ldp x21, x22, [x12, 16 * 1]
    ldp x23, x24, [x12, 16 * 2]
    ldp x25, x26, [x12, 16 * 3]
    ldp x27, x28, [x12, 16 * 4]
    ldp fp, lr, [x12, 16 * 5]
    ldr x9, [x12, 16 * 6]
    mov sp,  x9
    // msr tpidr_el1, x1
    ret
