/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <toolchain.h>
#include <linker/sections.h>

#include "../include/zvm_offsets_short_arch.h"
#include "../core/macro_priv.inc"

_ASM_FILE_PROLOGUE

/** VM entry point
 * x0:  vcpu info
 * x1:  host cpu context
 * x2:  zvm_hyp_context address
 */
GTEXT(guest_vm_entry)
SECTION_SUBSEC_FUNC(hyp_code, __hyp_section, guest_vm_entry)

	mrs	x4, sp_el0

	stp	x19, x20, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x19_20]
	stp	x21, x22, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x21_x22]
	stp	x23, x24, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x23_x24]
	stp	x25, x26, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x25_x26]
	stp	x27, x28, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x27_x28]
	stp	x29, x4,  [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x29_sp_el0]
    stp x18, x30, [x1, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x18_x30]

	mov x19, sp
	str x19, 	[x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_sp_elx]

    /* Error judgment */
    mrs	x1, isr_el1
	cbz	x1,  no_host_isr
	mov	x0, #0x0
	ret

no_host_isr:
    add x29, x0, #_vcpu_arch_to_ctxt
    ldr x29, [x29]

    ldp	x0, x1,   [x29, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x0_x1]
	ldp	x2, x3,   [x29, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x2_x3]
	ldp	x4, x5,   [x29, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x4_x5]
	ldp	x6, x7,   [x29, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x6_x7]
	ldp	x8, x9,   [x29, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x8_x9]
	ldp	x10, x11, [x29, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x10_x11]
	ldp	x12, x13, [x29, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x12_x13]
	ldp	x14, x15, [x29, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x14_x15]
	ldp	x16, x17, [x29, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x16_x17]
    ldp x18, x30, [x29, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x18_x30]

    mov x1, x29
	ldr x19, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_sp_elx]
	msr sp_el1, x19
	ldp x29, x19, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x29_sp_el0]
	msr sp_el0, x19

    ldp	x19, x20, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x19_20]
	ldp	x21, x22, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x21_x22]
	ldp	x23, x24, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x23_x24]
	ldp	x25, x26, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x25_x26]
	ldp	x27, x28, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x27_x28]
	ldr x4,		[x1, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x4_x5]
	ldr	x1,		[x1, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x0_x1+0x08]

	isb
    eret

/** VM exit point
 * x0:  vcpu info
 * x1:  the exception type
 */
GTEXT(guest_vm_exit)
SECTION_SUBSEC_FUNC(hyp_code, __hyp_section, guest_vm_exit)
	add x0, x0, #_vcpu_arch_to_ctxt
	ldr x0, [x0]

	stp	x2, x3,   [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x2_x3]
	stp	x4, x5,   [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x4_x5]
	stp	x6, x7,   [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x6_x7]
	stp	x8, x9,   [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x8_x9]
	stp	x10, x11, [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x10_x11]
	stp	x12, x13, [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x12_x13]
	stp	x14, x15, [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x14_x15]
	stp	x16, x17, [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x16_x17]
	stp	x19, x20, [x0, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x19_20]

	ldp x2, x3,   [sp], #16
    stp x2, x3,   [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x18_x30]
	ldp x2, x3,   [sp], #16
	stp	x2, x3,   [x0, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x0_x1]

	mrs x19, sp_el1
	str x19, [x0, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_sp_elx]

	stp	x21, x22, [x0, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x21_x22]
	stp	x23, x24, [x0, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x23_x24]
	stp	x25, x26, [x0, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x25_x26]
	stp	x27, x28, [x0, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x27_x28]
	stp	x29, x4,  [x0, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x29_sp_el0]

	/* Save the vcpu info for next use */
	mov x9, x0
	mov x10, x1

#ifdef	CONFIG_ZVM_TIME_MEASURE
	mov x0, #0x0
	cmp x1, x0
	b.ne	no_irq_exit

	/* Record the start time */
	bl vm_exit_irq_time

no_irq_exit:
	mrs x0, esr_el2
	ubfx x1, x0, #26, #6
	mov x0, #0b010110
	cmp x1, x0
	b.ne	no_timing_process

	/* Record the end time */
	bl vm_entry_irq_time

no_timing_process:
#endif

	bl get_zvm_host_context

	mov x1, x11
	ldp	x19, x20, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x19_20]
	ldp	x21, x22, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x21_x22]
	ldp	x23, x24, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x23_x24]
	ldp	x25, x26, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x25_x26]
	ldp	x27, x28, [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x27_x28]
	ldp	x29, x4,  [x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_x29_sp_el0]

	ldr x18, 	[x1, #_zvm_vcpu_ctxt_arch_regs_to_callee_saved_sp_elx]
	mov sp, x18
	ldp x18, x30, [x1, #_zvm_vcpu_ctxt_arch_regs_to_esf_t_x18_x30]

	isb

	/* Return exception type code */
	mov x0, x10
	ret
