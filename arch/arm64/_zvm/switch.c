/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <kernel/sched_priq.h>
#include <arch/arm64/lib_helpers.h>
#include <arch/cpu.h>

#include <_zvm/arm/cpu.h>
#include <_zvm/arm/asm.h>
#include <_zvm/zvm.h>
#include <_zvm/arm/sysreg.h>
#include <_zvm/arm/switch.h>
#include <_zvm/arm/trap_handler.h>
#include <_zvm/arm/vgic_common.h>

#include <drivers/interrupt_controller/gic.h>
#include <arch/arm64/cpu.h>
#include <stdint.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

/* VM entry point */
extern int guest_vm_entry(struct vcpu *vcpu,struct zvm_vcpu_context *context);

static char *get_vhe_vector()
{
    extern void _vector_table();

    void *_vector_addr = _vector_table;

    return _vector_addr;
}

static void active_hyp_trap(struct vcpu *vcpu)
{
    uint64_t val;

    write_hcr_el2(vcpu->arch->hcr_el2);
    val = read_cpacr_el1();
    val |= CPACR_EL1_TTA;
    val &= ~CPACR_EL1_ZEN;
    val |= CPTR_EL2_TAM;
    val |= CPACR_EL1_FPEN_NOTRAP;

    write_cpacr_el1(val);
    write_vbar_el2((uint64_t)_hyp_vector_table);
}

static void inactive_hyp_trap(struct vcpu *vcpu)
{
    char* vector = NULL;

    if (vcpu->arch->hcr_el2 & HCR_VSE_BIT) {
        vcpu->arch->hcr_el2 = read_hcr_el2();
    }

    write_hcr_el2(HCR_VHE_FLAGS);
    vector = get_vhe_vector();
    write_vbar_el2((uint64_t)vector);
}

static void vm_disable_daif(void)
{
    disable_debug_exceptions();
    disable_serror_exceptions();
    disable_fiq();

    disable_irq();
    isb();
}

static void vm_enable_daif(void)
{
    enable_debug_exceptions();
    enable_fiq();
    enable_serror_exceptions();

    enable_irq();
    isb();
}

static void save_host_context(struct zvm_vcpu_context *h_ctxt, struct vcpu *vcpu)
{
    h_ctxt->running_vcpu = vcpu;
    h_ctxt->sys_regs[VCPU_SPSR_EL1] = read_spsr_el1();

    /* for debugging. */
    h_ctxt->sys_regs[VCPU_MDSCR_EL1] = read_mdscr_el1();
    h_ctxt->regs.callee_saved_regs.sp_el0 = read_sp_el0();
}

static void load_host_context(struct zvm_vcpu_context *h_ctxt)
{
    write_mdscr_el1(h_ctxt->sys_regs[VCPU_MDSCR_EL1]);
    write_sp_el0(h_ctxt->regs.callee_saved_regs.sp_el0);

    write_spsr_el1(h_ctxt->sys_regs[VCPU_SPSR_EL1]);
}

static void vm_save_pgd(struct vm_arch *arch)
{
    arch->vtcr_el2 = read_vtcr_el2();
    arch->vttbr = read_vttbr_el2();
    isb();
}

static void vm_load_pgd(struct vm_arch *arch)
{
    write_vtcr_el2(arch->vtcr_el2);
    write_vttbr_el2(arch->vttbr);
    isb();
}

static int vm_flush_vgic(struct vcpu *vcpu)
{
    int ret = 0;
    ret = virt_irq_flush_vgic(vcpu);
    if (ret) {
        ZVM_LOG_ERR("Flush vgic info failed, Unknow reason \n");
    }
    return ret;
}

static int vm_sync_vgic(struct vcpu *vcpu)
{
    int ret = 0;

    ret = virt_irq_sync_vgic(vcpu);
    if (ret) {
        ZVM_LOG_ERR("Sync vgic info failed, Unknow reason \n");
    }
    return ret;
}

static int arch_vm_irq_trap(struct vcpu *vcpu)
{
    ARG_UNUSED(vcpu);
    /* enable all execption */
    vm_enable_daif();
    return 0;
}

static void arch_vm_serror_trap(struct vcpu *vcpu, int exit_code)
{
    uint64_t disr;
    uint64_t esr;

    if (ARM_VM_SERROR_PENDING(exit_code)) {
        disr = vcpu->arch->fault.disr_el1;

        esr = (0x2f << 26);
        if(disr & BIT(24))
            esr |= (disr & ((1<<25) - 1));
        else
            esr |= (disr & (0x7<<10 | 0x1<<9 | 0x3f));
    }
}

/**
 * @brief Get the zvm host context object for context switch
 */

void get_zvm_host_context(void)
{
    uint64_t hostctxt_addr ;
    struct k_thread *thread = _current;
    struct vcpu *vcpu = thread->vcpu_struct;

    hostctxt_addr = (uint64_t)&(vcpu->arch->host_ctxt);
    __asm__ volatile(
        "mov x11, %0"
        : : "r" (hostctxt_addr) :
    );
}

/**
 * @brief run the virtual machine on the specified vcpu.
 */

int arch_vcpu_run(struct vcpu *vcpu)
{
    int ret;
    uint64_t exit_type;
    struct zvm_vcpu_context *h_ctxt;

    /* mask all interrupt here to disable interrupt */
    vm_disable_daif();
    ret = vm_flush_vgic(vcpu);
    if (ret) {
        return ret;
    }

    h_ctxt = &vcpu->arch->host_ctxt;
    h_ctxt->regs.lr = GET_HOST_LR;
    save_host_context(h_ctxt, vcpu);

    vm_load_pgd(vcpu->vm->arch);
    active_hyp_trap(vcpu);
    vcpu->arch->host_ctxt.sys_regs[VCPU_TPIDRRO_EL0] = read_tpidrro_el0();
    write_tpidrro_el0(vcpu->arch->ctxt.sys_regs[VCPU_TPIDRRO_EL0]);
    vm_sysreg_load(&vcpu->arch->ctxt);

    exit_type = guest_vm_entry(vcpu, h_ctxt);

    vcpu->arch->ctxt.sys_regs[VCPU_TPIDRRO_EL0] = read_tpidrro_el0();
    write_tpidrro_el0(vcpu->arch->host_ctxt.sys_regs[VCPU_TPIDRRO_EL0]);
    vm_sysreg_save(&vcpu->arch->ctxt);
    inactive_hyp_trap(vcpu);
    vm_save_pgd(vcpu->vm->arch);

    h_ctxt = &vcpu->arch->host_ctxt;
    load_host_context(h_ctxt);

    vm_sync_vgic(vcpu);
    switch (exit_type) {
	case ARM_VM_EXCEPTION_SYNC:
        ret = arch_vm_trap_sync(vcpu);
        break;
    case ARM_VM_EXCEPTION_IRQ:
        ret = arch_vm_irq_trap(vcpu);
        break;
	case ARM_VM_EXCEPTION_SERROR:
        arch_vm_serror_trap(vcpu, exit_type);
	default:
		ZVM_LOG_WARN("Unsupported exception type in this stage....\n");
		return -ESRCH;
        break;
	}

    if (vcpu->vm->vm_status == VM_STATE_HALT) {
        ret = -ESRCH;
    }
    return ret;
}

/**
 * @brief Check before irq interrupt
 */

void z_vm_switch_handle_pre(uint32_t irq)
{
    struct k_thread *thread;
    struct vcpu *vcpu;

    if( (vcpu = _current_vcpu) == NULL){
        return;
    }

    bool *bit_addr = VGIC_DATA_IBP(vcpu->vm->vm_irq_block_data);
    /* If it is a vcpu thread, judge whether the signal is send to it */
    if(!bit_addr[irq]){
        return;
    }

    thread = vcpu->work->vcpu_thread;
    thread->base.thread_state |= _THREAD_VCPU_NO_SWITCH;
}
