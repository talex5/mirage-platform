/******************************************************************************
 * hypercall-x86_64.h
 * 
 * Copied from XenLinux.
 * 
 * Copyright (c) 2002-2004, K A Fraser
 * 
 * 64-bit updates:
 *   Benjamin Liu <benjamin.liu@intel.com>
 *   Jun Nakajima <jun.nakajima@intel.com>
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __HYPERCALL_ARM_H__
#define __HYPERCALL_ARM_H__

#include <xen/xen.h>
#include <xen/sched.h>
#include <mini-os/mm.h>

int
HYPERVISOR_mmu_update(
	mmu_update_t *req, int count, int *success_count, domid_t domid);

int
HYPERVISOR_mmuext_op(
	struct mmuext_op *op, int count, int *success_count, domid_t domid);

int
HYPERVISOR_set_gdt(
	unsigned long *frame_list, int entries);

int
HYPERVISOR_stack_switch(
	unsigned long ss, unsigned long esp);

int
HYPERVISOR_set_callbacks(
	unsigned long event_address, unsigned long failsafe_address, 
	unsigned long syscall_address);

int
HYPERVISOR_fpu_taskswitch(
	int set);

int
HYPERVISOR_sched_op(
	int cmd, void *arg);

static inline int
HYPERVISOR_shutdown(
	unsigned int reason)
{
	struct sched_shutdown shutdown = { .reason = reason };
	HYPERVISOR_sched_op(SCHEDOP_shutdown, &shutdown);
}

long
HYPERVISOR_set_timer_op(
	uint64_t timeout);

int
HYPERVISOR_set_debugreg(
	int reg, unsigned long value);

unsigned long
HYPERVISOR_get_debugreg(
	int reg);

int
HYPERVISOR_update_descriptor(
	unsigned long ma, unsigned long word);

int
HYPERVISOR_memory_op(
	unsigned int cmd, void *arg);

int
HYPERVISOR_multicall(
	void *call_list, int nr_calls);

int
HYPERVISOR_update_va_mapping(
	unsigned long va, pte_t new_val, unsigned long flags);

int
HYPERVISOR_event_channel_op(
       int cmd, void *op);

int
HYPERVISOR_xen_version(
	int cmd, void *arg);

int
HYPERVISOR_console_io(
	int cmd, int count, char *str);

int
HYPERVISOR_physdev_op(
	void *physdev_op);

int
HYPERVISOR_grant_table_op(
	unsigned int cmd, void *uop, unsigned int count);

int
HYPERVISOR_update_va_mapping_otherdomain(
	unsigned long va, pte_t new_val, unsigned long flags, domid_t domid);

int
HYPERVISOR_vm_assist(
	unsigned int cmd, unsigned int type);

int
HYPERVISOR_vcpu_op(
	int cmd, int vcpuid, void *extra_args);

int
HYPERVISOR_set_segment_base(
	int reg, unsigned long value);

int
HYPERVISOR_suspend(
	unsigned long srec);

int
HYPERVISOR_nmi_op(
	unsigned long op,
	unsigned long arg);

int
HYPERVISOR_sysctl(
	unsigned long op);

int
HYPERVISOR_domctl(
	unsigned long op);

int
HYPERVISOR_hvm_op(
	unsigned long op, void *arg);

#endif /* __HYPERCALL_X86_64_H__ */

/*
 * Local variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
