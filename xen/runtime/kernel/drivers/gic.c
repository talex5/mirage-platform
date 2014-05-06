// ARM GIC implementation

#include <mini-os/os.h>
#include <mini-os/hypervisor.h>
#include <libfdt.h>

//#define VGIC_DEBUG
#ifdef VGIC_DEBUG
#define DEBUG(_f, _a...) \
    printk("MINI_OS(file=gic.c, line=%d) " _f , __LINE__, ## _a)
#else
#define DEBUG(_f, _a...)    ((void)0)
#endif

extern void (*IRQ_handler)(void);

extern void *device_tree;

struct gic {
    volatile char *gicd_base;
    volatile char *gicc_base;
};

static struct gic gic;

// Distributor Interface
#define GICD_CTLR       0x0
#define GICD_ISENABLER  0x100
#define GICD_PRIORITY   0x400
#define GICD_ITARGETSR  0x800
#define GICD_ICFGR      0xC00

// CPU Interface
#define GICC_CTLR       0x0
#define GICC_PMR        0x4
#define GICC_IAR        0xc
#define GICC_EOIR       0x10
#define GICC_HPPIR      0x18

#define gicd(gic, offset) ((gic)->gicd_base + (offset))
#define gicc(gic, offset) ((gic)->gicc_base + (offset))

#define REG(addr) ((uint32_t *)(addr))

static inline uint32_t REG_READ32(volatile uint32_t *addr)
{
    uint32_t value;
    __asm__ __volatile__("ldr %0, [%1]":"=&r"(value):"r"(addr));
    rmb();
    return value;
}

static inline void REG_WRITE32(volatile uint32_t *addr, unsigned int value)
{
    __asm__ __volatile__("str %0, [%1]"::"r"(value), "r"(addr));
    wmb();
}

static void gic_set_priority(struct gic *gic, unsigned char irq_number, unsigned char priority)
{
    uint32_t value;
    value = REG_READ32(REG(gicd(gic, GICD_PRIORITY)) + irq_number);
    value &= ~(0xff << (8 * (irq_number & 0x3))); // set priority to '0'
    value |= priority << (8 * (irq_number & 0x3)); // add our priority
    REG_WRITE32(REG(gicd(gic, GICD_PRIORITY)) + irq_number, value);
}

static void gic_route_interrupt(struct gic *gic, unsigned char irq_number, unsigned char cpu_set)
{
    uint32_t value;
    value = REG_READ32(REG(gicd(gic, GICD_ITARGETSR)) + irq_number);
    value &= ~(0xff << (8 * (irq_number & 0x3))); // set priority to '0'
    value |= cpu_set << (8 * (irq_number & 0x3)); // add our priority
    REG_WRITE32(REG(gicd(gic, GICD_ITARGETSR)) + irq_number, value);
}

static void gic_enable_interrupt(struct gic *gic, unsigned char irq_number,
        unsigned char cpu_set, unsigned char level_sensitive, unsigned char ppi)
{
    void *set_enable_reg;
    void *cfg_reg;

    // set priority
    gic_set_priority(gic, irq_number, 0x0);

    // set target cpus for this interrupt
    gic_route_interrupt(gic, irq_number, cpu_set);

    // set level/edge triggered
    cfg_reg = (void *)gicd(gic, GICD_ICFGR);
    level_sensitive ? clear_bit((irq_number * 2) + 1, cfg_reg) : set_bit((irq_number * 2) + 1, cfg_reg);
    if(ppi)
        clear_bit((irq_number * 2), cfg_reg);

    wmb();

    // enable forwarding interrupt from distributor to cpu interface
    set_enable_reg = (void *)gicd(gic, GICD_ISENABLER);
    set_bit(irq_number, set_enable_reg);
    wmb();
}

static void gic_enable_interrupts(struct gic *gic)
{
    // Global enable forwarding interrupts from distributor to cpu interface
    REG_WRITE32(REG(gicd(gic, GICD_CTLR)), 0x00000001);

    // Global enable signalling of interrupt from the cpu interface
    REG_WRITE32(REG(gicc(gic, GICC_CTLR)), 0x00000001);
}

static void gic_disable_interrupts(struct gic *gic)
{
    // Global disable signalling of interrupt from the cpu interface
    REG_WRITE32(REG(gicc(gic, GICC_CTLR)), 0x00000000);

    // Global disable forwarding interrupts from distributor to cpu interface
    REG_WRITE32(REG(gicd(gic, GICD_CTLR)), 0x00000000);
}

static void gic_cpu_set_priority(struct gic *gic, char priority)
{
    REG_WRITE32(REG(gicc(gic, GICC_PMR)), priority & 0x000000FF);
}

static unsigned long gic_readiar(struct gic *gic) {
    return REG_READ32(REG(gicc(gic, GICC_IAR))) & 0x000003FF; // Interrupt ID
}

static void gic_eoir(struct gic *gic, uint32_t irq) {
    REG_WRITE32(REG(gicc(gic, GICC_EOIR)), irq & 0x000003FF);
}

//FIXME Get event_irq from dt
#define EVENTS_IRQ 31
#define VIRTUALTIMER_IRQ 27

//FIXME Move to a header file
void timer_handler(evtchn_port_t port, struct pt_regs *regs, void *ign);

static void gic_handler(void) {
    unsigned int irq = gic_readiar(&gic);

    DEBUG("IRQ received : %i\n", irq);
    switch(irq) {
    case EVENTS_IRQ:
        do_hypervisor_callback(NULL);
        break;
    case VIRTUALTIMER_IRQ:
        timer_handler(0, NULL, 0);
        break;
    default:
        DEBUG("Unhandled irq\n");
        break;
    }

    DEBUG("EIRQ\n");

    gic_eoir(&gic, irq);
}

void gic_init(void) {
    gic.gicd_base = NULL;
    int node = 0;
    int depth = 0;
    for (;;)
    {
        node = fdt_next_node(device_tree, node, &depth);
        if (node <= 0 || depth < 0)
            break;

        /*
           int name_len = 0;
           const char *name = fdt_get_name(device_tree, node, &name_len);
           printk("Found node: %d (%.*s)\n", node, name_len, name);
         */

        if (fdt_getprop(device_tree, node, "interrupt-controller", NULL)) {
            int len = 0;
            const uint64_t *reg = fdt_getprop(device_tree, node, "reg", &len);
            if (reg == NULL || len != 32) {
                printk("Bad 'reg' property: %p %d\n", reg, len);
                continue;
            }
            gic.gicd_base = (char *) (long) fdt64_to_cpu(reg[0]);
            gic.gicc_base = (char *) (long) fdt64_to_cpu(reg[2]);
            printk("Found GIC: gicd_base = %p, gicc_base = %p\n", gic.gicd_base, gic.gicc_base);
            break;
        }
    }
    if (!gic.gicd_base) {
        printk("GIC not found!\n");
        BUG();
    }
    wmb();

    IRQ_handler = gic_handler;

    gic_disable_interrupts(&gic);
    gic_cpu_set_priority(&gic, 0xff);

    /* Must call gic_enable_interrupts before enabling individual interrupts, otherwise our IRQ handler
     * gets called endlessly with spurious interrupts. */
    gic_enable_interrupts(&gic);

    gic_enable_interrupt(&gic, EVENTS_IRQ /* interrupt number */, 0x1 /*cpu_set*/, 1 /*level_sensitive*/, 0 /* ppi */);
    gic_enable_interrupt(&gic, VIRTUALTIMER_IRQ /* interrupt number */, 0x1 /*cpu_set*/, 1 /*level_sensitive*/, 1 /* ppi */);
}
