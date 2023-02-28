/*
 * Copyright (c) 2022 xcl (xiongcl@hnu.edu.cn)
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZVM_UART_H_
#define __ZVM_UART_H_

#include <zephyr.h>
#include <kernel.h>
#include <devicetree.h>

#include <_zvm/zvm.h>


#define PL011_UART0_BASE    DT_REG_ADDR_BY_IDX(DT_INST(0, arm_pl011), 0)
#define PL011_UART0_SIZE    DT_REG_SIZE_BY_IDX(DT_INST(0, arm_pl011), 0)
#define PL011_UART0_END     (PL011_UART0_BASE + PL011_UART0_SIZE)
#define PL011_UART0_IRQ     DT_IRQ_BY_IDX(DT_INST(0, arm_pl011), 0, irq)
#define PL011_UART0_NAME    DT_NODE_FULL_NAME(DT_INST(0, arm_pl011))

#ifdef CONFIG_SOC_FVP_CORTEX_A55

#define PL011_UART1_NAME    DT_NODE_FULL_NAME(DT_INST(1, arm_pl011))
#define PL011_UART1_BASE    DT_REG_ADDR_BY_IDX(DT_INST(1, arm_pl011), 0)
#define PL011_UART1_SIZE    DT_REG_SIZE_BY_IDX(DT_INST(1, arm_pl011), 0)
#define PL011_UART1_END     (PL011_UART1_BASE + PL011_UART1_SIZE)
#define PL011_UART1_IRQ     DT_IRQ_BY_IDX(DT_INST(1, arm_pl011), 0, irq)
#define PL011_UART1_DEV     DEVICE_DT_GET(DT_INST(1, arm_pl011))

#define ZVM_VM_UART0_NAME   PL011_UART1_NAME
#define ZVM_VM_UART0_BASE   PL011_UART1_BASE
#define ZVM_VM_UART0_SIZE   PL011_UART1_SIZE
#define ZVM_VM_UART0_END    PL011_UART1_END
#define ZVM_VM_UART0_IRQ    PL011_UART1_IRQ
#define ZVM_VM_UART0_DEV    PL011_UART1_DEV


#define PL011_UART2_NAME    DT_NODE_FULL_NAME(DT_INST(2, arm_pl011))
#define PL011_UART2_BASE    DT_REG_ADDR_BY_IDX(DT_INST(2, arm_pl011), 0)
#define PL011_UART2_SIZE    DT_REG_SIZE_BY_IDX(DT_INST(2, arm_pl011), 0)
#define PL011_UART2_END     (PL011_UART2_BASE + PL011_UART2_SIZE)
#define PL011_UART2_IRQ      DT_IRQ_BY_IDX(DT_INST(2, arm_pl011), 0, irq)
#define PL011_UART2_DEV     DEVICE_DT_GET(DT_INST(2, arm_pl011))

#define ZVM_VM_UART1_NAME   PL011_UART2_NAME
#define ZVM_VM_UART1_BASE   PL011_UART2_BASE
#define ZVM_VM_UART1_SIZE   PL011_UART2_SIZE
#define ZVM_VM_UART1_END    PL011_UART2_END
#define ZVM_VM_UART1_IRQ    PL011_UART2_IRQ
#define ZVM_VM_UART1_DEV    PL011_UART2_DEV


#define DEFAULT_VM_UART_BASE    PL011_UART0_BASE
#define DEFAULT_VM_UART_SIZE    PL011_UART0_SIZE
#define DEFAULT_VM_UART_IRQ     PL011_UART0_IRQ


#endif /* CONFIG_SOC_FVP_CORTEX_A55 */

#ifdef CONFIG_SOC_QEMU_CORTEX_MAX

#define PL011_UART1_NAME    DT_NODE_FULL_NAME(DT_INST(1, arm_pl011))
#define PL011_UART1_BASE    DT_REG_ADDR_BY_IDX(DT_INST(1, arm_pl011), 0)
#define PL011_UART1_SIZE    DT_REG_SIZE_BY_IDX(DT_INST(1, arm_pl011), 0)
#define PL011_UART1_END     (PL011_UART1_BASE + PL011_UART1_SIZE)
#define PL011_UART1_IRQ     DT_IRQ_BY_IDX(DT_INST(1, arm_pl011), 0, irq)
#define PL011_UART1_DEV     DEVICE_DT_GET(DT_INST(1, arm_pl011))

#define ZVM_VM_UART0_NAME   PL011_UART1_NAME
#define ZVM_VM_UART0_BASE   PL011_UART1_BASE
#define ZVM_VM_UART0_SIZE   PL011_UART1_SIZE
#define ZVM_VM_UART0_END    PL011_UART1_END
#define ZVM_VM_UART0_IRQ    PL011_UART1_IRQ
#define ZVM_VM_UART0_DEV    PL011_UART1_DEV

#define PL011_UART2_NAME    DT_NODE_FULL_NAME(DT_INST(2, arm_pl011))
#define PL011_UART2_BASE    DT_REG_ADDR_BY_IDX(DT_INST(2, arm_pl011), 0)
#define PL011_UART2_SIZE    DT_REG_SIZE_BY_IDX(DT_INST(2, arm_pl011), 0)
#define PL011_UART2_END     (PL011_UART2_BASE + PL011_UART2_SIZE)
#define PL011_UART2_IRQ      DT_IRQ_BY_IDX(DT_INST(2, arm_pl011), 0, irq)
#define PL011_UART2_DEV     DEVICE_DT_GET(DT_INST(2, arm_pl011))

#define ZVM_VM_UART1_NAME   PL011_UART2_NAME
#define ZVM_VM_UART1_BASE   PL011_UART2_BASE
#define ZVM_VM_UART1_SIZE   PL011_UART2_SIZE
#define ZVM_VM_UART1_END    PL011_UART2_END
#define ZVM_VM_UART1_IRQ    PL011_UART2_IRQ
#define ZVM_VM_UART1_DEV    PL011_UART2_DEV


#define DEFAULT_VM_UART_BASE    PL011_UART0_BASE
#define DEFAULT_VM_UART_SIZE    PL011_UART0_SIZE
#define DEFAULT_VM_UART_IRQ     PL011_UART0_IRQ

#endif /* CONFIG_SOC_QEMU_CORTEX_MAX */

/*Device defaule irq num is 0xffff, means not bind to vm*/
#define ZVM_VM_INVALID_UART_IRQ 0x0000ffff

#define ZVM_VM_UART_PRIO        (0x2)


/**
 * @brief Add idle uart dev info to the zvm total dev list.
 */
int zvm_add_uart_dev(struct zvm_dev_lists *dev_list);



#endif  /* __ZVM_UART_H_ */
