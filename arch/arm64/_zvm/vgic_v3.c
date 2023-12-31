/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <zephyr.h>
#include <device.h>
#include <kernel_structs.h>
#include <init.h>
#include <arch/cpu.h>
#include <arch/arm64/lib_helpers.h>
#include <arch/common/sys_bitops.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <drivers/interrupt_controller/gic.h>
#include <logging/log.h>
#include <../drivers/interrupt_controller/intc_gicv3_priv.h>

#include <_zvm/arm/cpu.h>
#include <_zvm/arm/vgic_v3.h>
#include <_zvm/arm/vgic_common.h>
#include <_zvm/zvm.h>
#include <_zvm/vm_irq.h>
#include <_zvm/vm_console.h>


LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

static struct arch_gicv3_info overall_gicv3_info;

/**
 * @brief load list register for vcpu interface.
 */
static void vgicv3_lrs_load(struct gicv3_vcpuif_ctxt *ctxt)
{
    uint32_t rg_cout = overall_gicv3_info.lr_nums;

    if (rg_cout > vgicv3_lrs_count) {
        ZVM_LOG_WARN("System list registers do not support! \n");
        return;
    }

	switch (rg_cout) {
	case 8:
		write_sysreg(ctxt->ich_lr7_el2, ICH_LR7_EL2);
	case 7:
		write_sysreg(ctxt->ich_lr6_el2, ICH_LR6_EL2);
	case 6:
		write_sysreg(ctxt->ich_lr5_el2, ICH_LR5_EL2);
	case 5:
		write_sysreg(ctxt->ich_lr4_el2, ICH_LR4_EL2);
	case 4:
		write_sysreg(ctxt->ich_lr3_el2, ICH_LR3_EL2);
	case 3:
		write_sysreg(ctxt->ich_lr2_el2, ICH_LR2_EL2);
	case 2:
		write_sysreg(ctxt->ich_lr1_el2, ICH_LR1_EL2);
	case 1:
		write_sysreg(ctxt->ich_lr0_el2, ICH_LR0_EL2);
		break;
	default:
		break;
	}
}

static void vgicv3_prios_load(struct gicv3_vcpuif_ctxt *ctxt)
{
    uint32_t rg_cout = overall_gicv3_info.prio_num;

    switch (rg_cout) {
	case 7:
		write_sysreg(ctxt->ich_ap0r2_el2, ICH_AP0R2_EL2);
		write_sysreg(ctxt->ich_ap1r2_el2, ICH_AP1R2_EL2);
	case 6:
		write_sysreg(ctxt->ich_ap0r1_el2, ICH_AP0R1_EL2);
		write_sysreg(ctxt->ich_ap1r1_el2, ICH_AP1R1_EL2);
	case 5:
		write_sysreg(ctxt->ich_ap0r0_el2, ICH_AP0R0_EL2);
		write_sysreg(ctxt->ich_ap1r0_el2, ICH_AP1R0_EL2);
		break;
	default:
		ZVM_LOG_ERR("Load prs error");
	}
}

static void vgicv3_ctrls_load(struct gicv3_vcpuif_ctxt *ctxt)
{
    write_sysreg(ctxt->icc_sre_el1, ICC_SRE_EL1);
	write_sysreg(ctxt->icc_ctlr_el1, ICC_CTLR_EL1);

	write_sysreg(ctxt->ich_vmcr_el2, ICH_VMCR_EL2);
	write_sysreg(ctxt->ich_hcr_el2, ICH_HCR_EL2);
}

static void vgicv3_lrs_save(struct gicv3_vcpuif_ctxt *ctxt)
{
	uint32_t rg_cout = overall_gicv3_info.lr_nums;

    if (rg_cout > vgicv3_lrs_count) {
        ZVM_LOG_WARN("System list registers do not support! \n");
        return;
    }

	switch (rg_cout) {
	case 8:
		ctxt->ich_lr7_el2 = read_sysreg(ICH_LR7_EL2);
	case 7:
		ctxt->ich_lr6_el2 = read_sysreg(ICH_LR6_EL2);
	case 6:
		ctxt->ich_lr5_el2 = read_sysreg(ICH_LR5_EL2);
	case 5:
		ctxt->ich_lr4_el2 = read_sysreg(ICH_LR4_EL2);
	case 4:
		ctxt->ich_lr3_el2 = read_sysreg(ICH_LR3_EL2);
	case 3:
		ctxt->ich_lr2_el2 = read_sysreg(ICH_LR2_EL2);
	case 2:
		ctxt->ich_lr1_el2 = read_sysreg(ICH_LR1_EL2);
	case 1:
		ctxt->ich_lr0_el2 = read_sysreg(ICH_LR0_EL2);
		break;
	default:
		break;
	}
}

static void vgicv3_prios_save(struct gicv3_vcpuif_ctxt *ctxt)
{
    uint32_t rg_cout = overall_gicv3_info.prio_num;

	switch (rg_cout) {
	case 7:
		ctxt->ich_ap0r2_el2 = read_sysreg(ICH_AP0R2_EL2);
		ctxt->ich_ap1r2_el2 = read_sysreg(ICH_AP1R2_EL2);
	case 6:
		ctxt->ich_ap0r1_el2 = read_sysreg(ICH_AP0R1_EL2);
		ctxt->ich_ap1r1_el2 = read_sysreg(ICH_AP1R1_EL2);
	case 5:
		ctxt->ich_ap0r0_el2 = read_sysreg(ICH_AP0R0_EL2);
		ctxt->ich_ap1r0_el2 = read_sysreg(ICH_AP1R0_EL2);
		break;
	default:
		ZVM_LOG_ERR(" Set ich_ap priority failed. \n");
	}
}

static void vgicv3_ctrls_save(struct gicv3_vcpuif_ctxt *ctxt)
{
    ctxt->icc_sre_el1 = read_sysreg(ICC_SRE_EL1);
	ctxt->icc_ctlr_el1 = read_sysreg(ICC_CTLR_EL1);

	ctxt->ich_vmcr_el2 = read_sysreg(ICH_VMCR_EL2);
	ctxt->ich_hcr_el2 = read_sysreg(ICH_HCR_EL2);
}


int gicv3_inject_virq(struct vcpu *vcpu, struct virt_irq_desc *desc)
{
	uint64_t value = 0;
	struct gicv3_lr *lr = (struct gicv3_lr *)&value;

	if (desc->id >= overall_gicv3_info.lr_nums) {
		ZVM_LOG_WARN("invalid virq id %d\n", desc->id);
		return -EINVAL;
	}
	lr->vINTID = desc->virq_num;
	lr->pINTID = desc->pirq_num;
	lr->priority = desc->prio;
	lr->group = 0x01;
	lr->hw = 0x01;  /* !!(desc->virq_flags & VIRT_IRQ_HW) */
	lr->state = 0x01;

	gicv3_write_lr(desc->id, value);
	return 0;
}

uint8_t gicv3_get_lr_state(struct vcpu *vcpu, struct virt_irq_desc *virq)
{
	uint64_t value;

	if (virq->id >=  overall_gicv3_info.lr_nums) {
		return 0;
	}
	value = gicv3_read_lr(virq->id);
	value = (value >> 62) & 0x03;

	return ((uint8_t)value);
}

int gicv3_update_lr(struct vcpu *vcpu, struct virt_irq_desc *desc, int action)
{
	ARG_UNUSED(vcpu);
	int ret = 0;
	if (!desc || desc->id >= overall_gicv3_info.lr_nums) {
		return -ENODEV;
	}

	switch (action) {
	case ACTION_CLEAR_VIRQ:
		gicv3_write_lr(desc->id, 0);
		break;

	default:
		ZVM_LOG_WARN(" Unsupported virq operation here. \n");
		ret = -ENODEV;
		break;
	}

	return ret;
}

int vgic_gicrsgi_mem_read(struct vcpu *vcpu, struct virt_gic_gicr *gicr,
                                uint32_t offset, uint64_t *v)
{
	uint32_t *value = (uint32_t *)v;

	switch (offset) {
	case GICR_SGI_CTLR:
		*value = gicr->gicr_ctlr & ~(1 << 31);
		break;
	case GICR_SGI_ISENABLER:
		*value = gicr->gicr_enabler0;
		break;
	case GICR_SGI_ICENABLER:
		*value = gicr->gicr_enabler0;
		break;
	case GICR_SGI_PENDING:
		*value = gicr->gicr_ispender;
		break;
	case GICR_SGI_PIDR2:
		*value = (0x03 << 4);
		break;
	default:
		*value = 0;
		break;
	}

	return 0;
}

int vgic_gicrsgi_mem_write(struct vcpu *vcpu, struct virt_gic_gicr *gicr, uint32_t offset, uint64_t *v)
{
	uint32_t *value = (uint32_t *)v;
	uint32_t mem_addr = (uint64_t)v;
	int bit;

	switch (offset) {
	case GICR_SGI_ISENABLER:
		vgic_irq_test_and_set_bit(vcpu, 0, value, 32, 1);
		for (bit = 0; bit < 32; bit++) {
			if (sys_test_bit(mem_addr, bit)) {
				gicr->gicr_enabler0 |= BIT(bit);
			}
		}
		break;
	case GICR_SGI_ICENABLER:
		vgic_irq_test_and_set_bit(vcpu, 0, value, 32, 0);
		for(bit = 0; bit < 32; bit++) {
			if (sys_test_bit(mem_addr, bit)) {
				gicr->gicr_enabler0 &= ~BIT(bit);
			}
		}
		break;
	case GICR_SGI_PENDING:
		/* clear pending state */
		for(bit = 0; bit < 32; bit++) {
			if (sys_test_bit(mem_addr, bit)) {
				sys_write32(BIT(bit), GIC_RDIST_BASE + GICR_SGI_BASE_OFF + GICR_SGI_PENDING);
				gicr->gicr_ispender &= ~BIT(bit);
			}
		}
		break;
	default:
		*value = 0;
		break;
	}

	return 0;
}

int vgic_gicrrd_mem_read(struct vcpu *vcpu, struct virt_gic_gicr *gicr, uint32_t offset, uint64_t *v)
{
	uint32_t *value = (uint32_t *)v;

	/* consider multiple cpu later, Now just return 0 */
	switch (offset) {
	case 0xffe8:
		*value = gicr->gicr_pidr2;
		break;
	case GICR_CTLR:
		*value = gicr->gicr_ctlr;
		break;
	case GICR_TYPER:
		*value = gicr->gicr_typer;
		*(value+1) = gicr->gicr_typer >> 32;
		break;
	case 0x000c:
		*value = gicr->gicr_typer >> 32;
		break;
	default:
		*value = 0;
		break;
	}

	return 0;
}

int vgic_gicrrd_mem_write(struct vcpu *vcpu, struct virt_gic_gicr *gicr, uint32_t offset, uint64_t *v)
{
	return 0;
}

/**
 * @brief Get gicr address type.
 */
int get_vcpu_gicr_type(struct virt_gic_gicr *gicr,
		uint32_t addr, uint32_t *offset)
{

	if ((addr >= gicr->vlpi_base) &&
		(addr < (gicr->vlpi_base + (64 * 1024)))) {

		*offset = addr - gicr->vlpi_base;
		return TYPE_GIC_GICR_VLPI;
	}

    if ((addr >= gicr->sgi_base) &&
		(addr < (gicr->sgi_base + (64 * 1024)))) {

		*offset = addr - gicr->sgi_base;
		return TYPE_GIC_GICR_SGI;
	}

    if ((addr >= gicr->raddr_base) &&
		(addr < (gicr->raddr_base + (64 * 1024)))) {

		*offset = addr - gicr->raddr_base;
		return TYPE_GIC_GICR_RD;
	}

	return TYPE_GIC_INVAILD;
}

int vgicv3_state_load(struct vcpu *vcpu, struct gicv3_vcpuif_ctxt *ctxt)
{
    vgicv3_lrs_load(ctxt);
    vgicv3_prios_load(ctxt);
    vgicv3_ctrls_load(ctxt);

	arch_vdev_irq_enable(vcpu);
	return 0;
}

int vgicv3_state_save(struct vcpu *vcpu, struct gicv3_vcpuif_ctxt *ctxt)
{
    vgicv3_lrs_save(ctxt);
    vgicv3_prios_save(ctxt);
    vgicv3_ctrls_save(ctxt);

	arch_vdev_irq_disable(vcpu);
	return 0;
}

/**
 * @brief Gic vcpu interface init .
 */
int vcpu_gicv3_init(struct gicv3_vcpuif_ctxt *ctxt)
{
    int ret = 0;
    ARG_UNUSED(ret);

    ctxt->icc_sre_el1 = 0x07;
	ctxt->icc_ctlr_el1 = read_sysreg(ICC_CTLR_EL1);
	ctxt->icc_ctlr_el1 |= 0x02;

    ctxt->ich_vmcr_el2 = GICH_VMCR_VENG1 | GICH_VMCR_DEFAULT_MASK;
    ctxt->ich_hcr_el2 = GICH_HCR_EN;

    return 0;
}

static int vdev_gicv3_init(struct vm *vm, struct vgicv3_dev *gicv3_vdev, uint32_t gicd_base, uint32_t gicd_size,
                            uint32_t gicr_base, uint32_t gicr_size)
{
    uint32_t spi_num;
    int i = 0;
    uint64_t tmp_typer = 0;
    struct virt_gic_gicd *gicd = &gicv3_vdev->gicd;
    struct virt_gic_gicr *gicr;
    struct vcpu *vcpu;

    if(!gicd){
		return -ENODEV;
	}
	ZVM_SPINLOCK_INIT(&gicd->gicd_lock);

    gicd->addr_base = gicd_base;
    gicd->addr_size = gicd_size;
    gicd->gicd_ctlr = 0;
    gicd->gicd_pidr2 = (0x3<<4);

    tmp_typer |= vm->vcpu_num << 5;
    tmp_typer |= 9 << 19;
    spi_num = (VM_GLOBAL_VIRQ_NR + 32) >> 5;
    spi_num -= 1;
    tmp_typer |= spi_num;

    gicd->gicd_typer = tmp_typer;

    for (i = 0; i < vm->vcpu_num; i++) {
        vcpu = vm->vcpus[i];
        gicr = (struct virt_gic_gicr *)k_malloc(sizeof(struct virt_gic_gicr));
        if(!gicr){
			return -EMMAO;
		}
        gicr->vcpu_id = vcpu->vcpu_id;
        gicr_base = gicr_base + (128 * 1024) * vcpu->vcpu_id;
        gicr->raddr_base = gicr_base;
        gicr->sgi_base = gicr_base + (64 * 1024);
        gicr->vlpi_base = 0;

        gicr->gicr_ctlr = 0;
        gicr->gicr_ispender = 0;
        gicr->gicr_typer = (uint64_t)vcpu->vcpu_id << 32;
        gicr->gicr_pidr2 = 0x3 << 4;

		gicv3_vdev->gicr[i] = gicr;
    }
	/* set last gicr region flag here, means it is the last gicr region */
	gicr->gicr_typer |= GICR_TYPER_LAST_FLAG;

	return 0;
}

struct vgicv3_dev *vgicv3_dev_init(struct vm *vm)
{
	int ret = 0;
    uint64_t gicd_base, gicd_size, gicr_base, gicr_size;
    struct vgicv3_dev *gicv3_vdev;

    gicd_base = VGIC_DIST_BASE;
    gicd_size = VGIC_DIST_SIZE;
    gicr_base = VGIC_RDIST_BASE;
    gicr_size = VGIC_RDIST_SIZE;
    if (!gicd_base || !gicd_size || !gicr_base || !gicr_size) {
        ZVM_LOG_ERR("GIC device has some error!");
        return NULL;
    }

    gicv3_vdev = (struct vgicv3_dev *)k_malloc(sizeof(struct vgicv3_dev));
    if (!gicv3_vdev) {
        ZVM_LOG_ERR("Allocat memory for gicv3 error \n");
        return NULL;
    }

	gicv3_vdev->v_dev.dev_pt_flag=false;
	gicv3_vdev->v_dev.shareable=false;
    ret = vm_virt_dev_add(vm, &gicv3_vdev->v_dev, "vgic_v3_device", gicd_base, gicd_base, gicd_size+gicr_size);
    if (ret) {
        ZVM_LOG_ERR("Init virt dev error \n");
        return NULL;
    }

    ret = vdev_gicv3_init(vm, gicv3_vdev, gicd_base, gicd_size, gicr_base, gicr_size);
    if(ret){
        ZVM_LOG_ERR("Init virt gicv3 error \n");
        return NULL;
    }

    gicv3_vdev->v_dev.vm_vdev_read = vgic_vdev_mem_read;
    gicv3_vdev->v_dev.vm_vdev_write = vgic_vdev_mem_write;
	return gicv3_vdev;
}

int vgicv3_ctrlblock_create(struct device *unused, struct vm *vm)
{
    ARG_UNUSED(unused);
	int ret = 0;
	uint32_t gic_flags = 0;
	struct vm_vgic_block *this_block;

	this_block = (struct vm_vgic_block *)k_malloc(sizeof(struct vm_vgic_block));
	if (!this_block) {
		ZVM_LOG_ERR("Allocat memory for vm_irq_block error \n");
        return -ENODEV;
	}

	if (overall_gicv3_info.lr_nums != 0) {
		gic_flags |= VIRQ_HW_SUPPORT;
		this_block->flags = gic_flags;
	} else {
		ZVM_LOG_ERR("Init gicv3 failed, the hardware do not supporte it. \n");
		return -ENODEV;
	}

	/* allocate virq desc for each spi */
    this_block->vm_virt_irq_desc = (struct virt_irq_desc *)k_malloc(VM_SPI_VIRQ_NR * sizeof(struct virt_irq_desc));
    if (this_block->vm_virt_irq_desc == NULL) {
        ZVM_LOG_WARN("Allocate virt_irq_desc struct error!");
        return -EMMAO;
    }

	this_block->enabled = false;
	this_block->cpu_num = CONFIG_MAX_VCPU_PER_VM;
	this_block->irq_num = VM_GLOBAL_VIRQ_NR;
	memset(&this_block->irq_target, 0, sizeof(uint32_t)*VM_SPI_VIRQ_NR);
	memset(this_block->sgi_vcpu_source, 0, sizeof(uint32_t)*CONFIG_MP_NUM_CPUS*VM_SGI_VIRQ_NR);
	memset(this_block->irq_bitmap, 0, VM_GLOBAL_VIRQ_NR/0x08);

	vm->vm_irq_block_data = this_block;
	return 0;

}

int vm_intctrl_vdev_create(struct vm *vm)
{
	int ret = 0;
	struct vgicv3_dev *gicv3_vdev;

	gicv3_vdev = vgicv3_dev_init(vm);
	if (!gicv3_vdev) {
		ZVM_LOG_ERR("Init gicv3 dev failed. \n");
		return -ENODEV;
	}
	VGIC_DATA_VGD(vm->vm_irq_block_data) = gicv3_vdev;

	return ret;
}

/**
 * @brief Init vgicv3 when ZVM module init.
 */
int arch_vgicv3_init(void *op)
{
	ARG_UNUSED(op);
	uint64_t ich_vtr_el2;

	ich_vtr_el2 = read_sysreg(ICH_VTR_EL2);
    overall_gicv3_info.lr_nums = (ich_vtr_el2 & 0x1f) + 1;
    overall_gicv3_info.prio_num = ((ich_vtr_el2>>29) & 0x7) + 1;
    overall_gicv3_info.ich_vtr_el2 = ich_vtr_el2;

	return 0;
}
