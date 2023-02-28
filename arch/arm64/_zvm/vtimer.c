/*
 * Copyright (c) 2021-2022 hnu <xiongcl@hnu.edu.cn>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <timeout_q.h>
#include <kernel_structs.h>
#include <drivers/timer/arm_arch_timer.h>
#include <drivers/timer/system_timer.h>
#include <arch/arm64/timer.h>

#include <_zvm/arm/vtimer.h>
#include <_zvm/arm/vgic_common.h>
#include <_zvm/vm.h>
#include <_zvm/zvm.h>
#include <_zvm/vm_irq.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

/* Global timer info */
static struct zvm_arch_timer_info zvm_global_vtimer_info;

static struct k_spinlock virt_vtimer_lock;
static struct k_spinlock virt_ptimer_lock;

/**
 * @brief Initializes _timeout struct for virtual timer
 */
static inline void init_virt_timer_timeout(struct _timeout *timeout, void *func)
{
	timeout->dticks = 0;
	timeout->fn = func;
	sys_dnode_init(&timeout->node);
}

/**
 * @brief Get the global timer information and pass it on vcpu.
 */
static inline void get_global_timer_info(struct virt_timer_context *vtimer_ctxt)
{
	vtimer_ctxt->virt_virq = zvm_global_vtimer_info.virt_irq;
	vtimer_ctxt->virt_pirq = zvm_global_vtimer_info.phys_irq;
}

/**
 * @brief Virtual vtimer isr function for process irq.
 */
static int arm_arch_virt_vtimer_compare_isr(void *dev)
{
	ARG_UNUSED(dev);
	int ret;
	uint32_t cntvctl;
	k_spinlock_key_t key = k_spin_lock(&virt_vtimer_lock);
	struct vcpu *vcpu = _current_vcpu;
	struct virt_timer_context *ctxt = vcpu->arch->vtimer_context;

	cntvctl = read_cntv_ctl_el02();
	if(!(cntvctl & CNTV_CTL_ISTAT_BIT)){
		ZVM_LOG_WARN("No virt vtimer interrupt but signal raise! \n");
		return -EVIRQ;
	}
	ctxt->cntv_ctl = cntvctl | CNTV_CTL_IMASK_BIT;

	ret = set_virq_to_vcpu(vcpu, ctxt->virt_virq);
	if(ret){
		k_spin_unlock(&virt_vtimer_lock, key);
		return ret;
	}

	k_spin_unlock(&virt_vtimer_lock, key);

	return 0;
}

/**
 * @brief Virtual ptimer isr function for process irq.
 */
static int arm_arch_virt_ptimer_compare_isr(void *dev)
{
	ARG_UNUSED(dev);
	int ret;
	k_spinlock_key_t key = k_spin_lock(&virt_ptimer_lock);
	struct vcpu *vcpu = _current_vcpu;
	struct virt_timer_context *ctxt = vcpu->arch->vtimer_context;

	ret = set_virq_to_vcpu(vcpu, ctxt->virt_pirq);
	if(ret){
		k_spin_unlock(&virt_ptimer_lock, key);
		return ret;
	}

	k_spin_unlock(&virt_ptimer_lock, key);

	return 0;
}

/**
 * @brief Processing virtual vtimer timeout for vm.
 */
static void virt_vtimer_expiry(struct _timeout *t)
{
	int virq_num = zvm_global_vtimer_info.virt_irq;
	struct virt_timer_context *ctxt;
	struct vcpu *vcpu;

	ctxt = CONTAINER_OF(t, struct virt_timer_context, vtimer_timeout);
	if(ctxt == NULL){
		ZVM_LOG_WARN("The virt_vtimer context is not exist! \n");
		return;
	}

	ctxt->cntv_ctl |= CNTV_CTL_IMASK_BIT;
	vcpu = (struct vcpu*)ctxt->vcpu;

	set_virq_to_vcpu(vcpu, virq_num);
}

/**
 * @brief Processing virtual ptimer timeout for vm.
 */
static void virt_ptimer_expiry(struct _timeout *t)
{
	int virq_num = zvm_global_vtimer_info.phys_irq;
	struct virt_timer_context *ctxt;
	struct vcpu *vcpu;

	ctxt = CONTAINER_OF(t, struct virt_timer_context, ptimer_timeout);
	if(ctxt == NULL){
		ZVM_LOG_WARN("The virt_ptimer context is not exist! \n");
		return;
	}

	ctxt->cntp_ctl |= CNTV_CTL_IMASK_BIT;
	vcpu = (struct vcpu*)ctxt->vcpu;

	set_virq_to_vcpu(vcpu, virq_num);
}

/**
 * @brief Simulate cntp_tval_el0 register
 */
void simulate_timer_cntp_tval(struct vcpu *vcpu, int read, uint64_t *value)
{
	uint64_t cycles;
	struct virt_timer_context *ctxt;

	ctxt = vcpu->arch->vtimer_context;
	cycles = arm_arch_timer_count() - ctxt->timer_offset;

	if (read) {
#ifdef CONFIG_HAS_ARM_VHE_EXTN
		*value = read_cntp_tval_el02();
#else
		uint64_t ns;
		ns = (ctxt->cntp_tval - cycles - ctxt->timer_offset) & 0xffffffff;
		*value = ns;
#endif
	} else {
#ifdef CONFIG_HAS_ARM_VHE_EXTN
		write_cntp_tval_el02(*value);
#else
		ctxt->cntp_cval = arm_arch_timer_count() + *value;
#endif
	}
}

/**
 * @brief Simulate cntp_cval_el0 register
 */
void simulate_timer_cntp_cval(struct vcpu *vcpu, int read, uint64_t *value)
{
	unsigned long ns;
	k_timeout_t vticks;
	struct virt_timer_context *ctxt;

	ctxt = vcpu->arch->vtimer_context;

	if (read) {
#ifdef CONFIG_HAS_ARM_VHE_EXTN
		*value = read_cntp_cval_el02();
#else
		*value = ctxt->cntp_cval;
#endif
	} else {
#ifdef CONFIG_HAS_ARM_VHE_EXTN
		ARG_UNUSED(ns);
		ARG_UNUSED(vticks);
		write_cntp_cval_el02(*value);
		ctxt->cntp_cval = read_cntp_cval_el02();
#else
		ctxt->cntp_cval = *value + ctxt->timer_offset;
		if (ctxt->cntp_ctl & CNTV_CTL_ENABLE_BIT) {
			ctxt->cntp_ctl &= ~CNTV_CTL_ISTAT_BIT;
			vticks.ticks = (ctxt->cntp_cval + ctxt->timer_offset)/HOST_CYC_PER_TICK;
			z_add_timeout(&ctxt->ptimer_timeout, ctxt->ptimer_timeout.fn, vticks);
		}
#endif
	}
}

/**
 * @brief Simulate cntp_ctl register
 */
void simulate_timer_cntp_ctl(struct vcpu *vcpu, int read, uint64_t *value)
{
	uint32_t reg_value = (uint32_t)(*value);
	k_timeout_t vticks;
	struct virt_timer_context *ctxt;

	ctxt = vcpu->arch->vtimer_context;

	if (read) {
#ifdef CONFIG_HAS_ARM_VHE_EXTN
		ARG_UNUSED(reg_value);
		ARG_UNUSED(vticks);
		*value = read_cntp_ctl_el02();
#else
		*value = ctxt->cntp_ctl;
#endif
	} else {
#ifdef CONFIG_HAS_ARM_VHE_EXTN
		write_cntp_ctl_el02(*value);
		ctxt->cntp_ctl = read_cntp_ctl_el02();
		/* TODO: Add softirq support*/
#else
		reg_value &= ~CNTV_CTL_ISTAT_BIT;

		if (reg_value & CNTV_CTL_ENABLE_BIT)
			reg_value |= ctxt->cntp_ctl & CNTV_CTL_ISTAT_BIT;
		ctxt->cntp_ctl = reg_value;

		if ((ctxt->cntp_ctl & CNTV_CTL_ENABLE_BIT) && (ctxt->cntp_cval != 0)) {
			vticks.ticks = (ctxt->cntp_cval + ctxt->timer_offset)/HOST_CYC_PER_TICK;
			z_add_timeout(&ctxt->ptimer_timeout, ctxt->ptimer_timeout.fn, vticks);
		}
#endif
	}
}

/**
 * @brief Initializes the virtual timer context for the vcpu:
 * This needs to be done when the vcpu is created. The step is below:
 * 1. Init vtimer and ptimer register.
 * 2. Add a timer expiry function for vcpu.
 * 3. Add a callbak function.
 */
void arch_vcpu_timer_init(struct vcpu *vcpu)
{
	struct virt_timer_context *ctxt;
	struct vm *vm;

	/* Default vcpu, get the count as offset */
	if (vcpu->vcpu_id == 0) {
		vcpu->vm->vtimer_offset = arm_arch_timer_count();
	}

    ctxt = vcpu->arch->vtimer_context;
	ctxt->vcpu = vcpu;
	ctxt->timer_offset = vcpu->vm->vtimer_offset;
	ctxt->enable_flag = false;

	/* init virt_timer struct */
	ctxt->cntv_ctl = CNTV_CTL_IMASK_BIT;
	ctxt->cntv_cval = 0;
	ctxt->cntv_tval = 0;
	ctxt->cntp_ctl = CNTV_CTL_IMASK_BIT;
	ctxt->cntp_cval = 0;
	ctxt->cntp_tval = 0;

	/* get virt timer irq */
	get_global_timer_info(ctxt);

	init_virt_timer_timeout(&ctxt->vtimer_timeout, virt_vtimer_expiry);
	init_virt_timer_timeout(&ctxt->ptimer_timeout, virt_ptimer_expiry);

	vm = vcpu->vm;
	bool *bit_addr = VGIC_DATA_IBP(vm->vm_irq_block_data);
	bit_addr[ctxt->virt_virq] = true;

	bit_addr = VGIC_DATA_IBP(vm->vm_irq_block_data);
	bit_addr[ctxt->virt_pirq] = true;
}

/**
 * @brief Get virtual timer irq number, it should be done when ZVM init.
 */
int zvm_arch_vtimer_init()
{
    /* get vtimer irq */
    zvm_global_vtimer_info.virt_irq = ARM_ARCH_VIRT_VTIMER_IRQ;
	zvm_global_vtimer_info.phys_irq = ARM_ARCH_VIRT_PTIMER_IRQ;

    if( (zvm_global_vtimer_info.virt_irq > 32) || (zvm_global_vtimer_info.virt_irq < 0)){
        ZVM_LOG_ERR("Can not get vtimer virt struct from hw. \n");
		return -EVIRQ;
    }

	if( (zvm_global_vtimer_info.phys_irq > 32) || (zvm_global_vtimer_info.phys_irq < 0)){
        ZVM_LOG_ERR("Can not get vtimer phys struct from hw. \n");
		return -EVIRQ;
    }

	IRQ_CONNECT(ARM_ARCH_VIRT_VTIMER_IRQ, ARM_ARCH_VIRT_VTIMER_PRIO,
		    arm_arch_virt_vtimer_compare_isr, NULL, ARM_ARCH_VIRT_VTIMER_FLAGS);

	IRQ_CONNECT(ARM_ARCH_VIRT_PTIMER_IRQ, ARM_ARCH_VIRT_PTIMER_PRIO,
		    arm_arch_virt_ptimer_compare_isr, NULL, ARM_ARCH_VIRT_PTIMER_FLAGS);

	return 0;
}
