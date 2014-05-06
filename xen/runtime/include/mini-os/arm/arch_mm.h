#ifndef _ARCH_MM_H_
#define _ARCH_MM_H_

extern char _text, _etext, _erodata, _edata, _end, __bss_start;
extern char stack[];

#define PAGE_SHIFT		12
#define PAGE_SIZE		(1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

#define L1_PAGETABLE_SHIFT      12

#if 0
#define VIRT_START                 ((unsigned long)&_text)
#else
#define VIRT_START                 ((unsigned long)0)
#endif

#define to_phys(x)                 ((unsigned long)(x)-VIRT_START)
#define to_virt(x)                 ((void *)((unsigned long)(x)+VIRT_START))

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> L1_PAGETABLE_SHIFT)
#define PFN_DOWN(x)	((x) >> L1_PAGETABLE_SHIFT)
#define PFN_PHYS(x)	((uint64_t)(x) << L1_PAGETABLE_SHIFT)
#define PHYS_PFN(x)	((x) >> L1_PAGETABLE_SHIFT)

#define virt_to_pfn(_virt)         (PFN_DOWN(to_phys(_virt)))
#define virt_to_mfn(_virt)         (PFN_DOWN(to_phys(_virt)))
#define mach_to_virt(_mach)        (_mach)
#define virt_to_mach(_virt)        (_virt)
#define mfn_to_virt(_mfn)          (to_virt(PFN_PHYS(_mfn)))
#define pfn_to_virt(_pfn)          (to_virt(PFN_PHYS(_pfn)))

// FIXME
#define map_frames(f, n) (NULL)

#endif
