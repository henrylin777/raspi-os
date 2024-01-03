#include "gpio.h"
#include "arm.h"
#include "sche.h"

/* Interrupt Request (IRQ) 
 * from page 112, BCM2837 ARM Peripherals, https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf
 */

#define IRQ_BASIC_PENDING		((volatile unsigned int *) (MMIO_BASE + 0x0000B200))
#define IRQ_PENDING_1		    ((volatile unsigned int *) (MMIO_BASE + 0x0000B204))
#define IRQ_PENDING_2		    ((volatile unsigned int *) (MMIO_BASE + 0x0000B208))
#define FIQ_CONTROL		        ((volatile unsigned int *) (MMIO_BASE + 0x0000B20C))
#define ENABLE_IRQS_1		    ((volatile unsigned int *) (MMIO_BASE + 0x0000B210))
#define ENABLE_IRQS_2		    ((volatile unsigned int *) (MMIO_BASE + 0x0000B214))
#define ENABLE_BASIC_IRQS		((volatile unsigned int *) (MMIO_BASE + 0x0000B218))
#define DISABLE_IRQS_1		    ((volatile unsigned int *) (MMIO_BASE + 0x0000B21C))
#define DISABLE_IRQS_2		    ((volatile unsigned int *) (MMIO_BASE + 0x0000B220))
#define DISABLE_BASIC_IRQS		((volatile unsigned int *) (MMIO_BASE + 0x0000B224))
#define CORE0_TIMER_IRQ_CTRL	((volatile unsigned int *) (0x40000040))
#define CORE0_INTERRUPT_SOURCE	((volatile unsigned int *) (0x40000060))

/* disable all interrupt */
#define disable_interrupt() asm volatile("msr DAIFSet, 0xf")
/* enable all interrupt */
#define enable_interrupt() asm volatile("msr DAIFClr, 0xf")

typedef void (*irq_handler_t)();

typedef struct irqnode {
    irq_handler_t handler;
    void *data;
    unsigned long prio; /* for now prio is ignored */
    list_node_t node;
} irq_t;

void show_task_info(task_struct *task);
void show_current_el();
void enable_timer_interrupt_el1();
void irq_init();
