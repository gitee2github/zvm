ZVM 总体设计概览 
================

ZVM是基于实时操作系统Zephyr设计的虚拟机管理器（Hypervisor），系统架构如下图所示，主要功能模块包括包括CPU虚拟化、
内存虚拟化、中断虚拟化、时钟虚拟化及I/O虚拟化等功能。
同时，现阶段Hypervisor支持两个VM，分别是Linux VM和Zephyr VM。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/refactor/zvm_doc/figure/overview.png
   :alt: Overview


基础模块
------------

处理器虚拟化
^^^^^^^^^^^^^^^^^^^^^^

CPU 虚拟化模块的主要功能是为每个 VM 的 vCPU 虚拟出一个单独的隔离上下文。 
在此系统中，每个vCPU都是作为一个线程存在的，由Hypervisor统一调度。 
为了提高vCPU的性能，ARMv8架构为Hypervisor提供了VHE支持，VHE可以使Host OS迁移到EL2特权模式，
而无需改变操作系统原有的代码。 VHE支持主要实现了ARM寄存器的重定向，这样我们就可以在不修改OS内核代码的情况下，
将Zephyr OS迁移到EL2层充当Hypervisor，既降低了系统冗余，又提高了系统性能。

内存虚拟化模块
^^^^^^^^^^^^^^^^^^^^^^

内存虚拟化模块的主要作用是实现VM内存地址的隔离。 系统需要隔离不同虚拟机的内存空间，
监控VM对实际物理内存的访问，以保护物理内存。 为了实现这个功能，ARM提供了两阶段的地址查找策略。 
第一阶段是从VM的虚拟地址到VM的物理地址的转换，第二阶段是从VM的物理地址到主机的物理地址。 
ARM 专门为 stage-2 翻译提供单独的硬件，以提高地址翻译性能。

中断虚拟化模块
^^^^^^^^^^^^^^^^^^^^^^

中断虚拟化使用ARM通用中断控制器（GIC）设备，并基于该设备实现虚拟中断配置。 
在此系统中，我们将 VM 的中断统一路由到 Hypervisor，Hypervisor 稍后会将它们分配给不同的 vCPU。 
具体实现上，虚拟中断的注入是通过GIC中的Virtual CPU接口或List Register实现的。

定时器虚拟化
^^^^^^^^^^^^^^^^^^^^^^

定时器虚拟化为每个 CPU 定义了一组虚拟定时器寄存器，它们在预定时间后单独计数并抛出中断，
由主机转发给 VM。 同时，在VM切换过程中，虚拟定时器会计算VM的实际运行时间，
并对虚拟机退出的时间进行补偿，为虚拟机提供定时器服务。

设备虚拟化
^^^^^^^^^^^^^^^^^^^^^^

在设备虚拟化方面，本文采用ARM中的Memory-mapped I/O(MMIO)方法将设备地址映射到虚拟的内存地址，构建虚拟的设备空间，
实现VN对设备地址的访问。 在具体实现上，我们统一构建一个虚拟的MMIO设备，在VM创建过程中将该设备分配给指定的VM，
实现I/O虚拟化。此外，对一些非独占设备，我们使用设备直通的方式实现设备的访问。


`Next>> 主机开发环境搭建 <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/2_Environment_Configuration.rst>`__
