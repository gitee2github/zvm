#!/bin/bash

# operation string and platform string
OPS=$1
PLAT=$2

ops_build="build"
ops_debug="debugserver"

plat_qemu="qemu"
plat_fvp="fvp"
plat_rk3568="rk3568"

# Build system
if [ "$OPS" = "$ops_build" ]; then
    rm -rf build/

    if [ "$PLAT" = "$plat_qemu" ]; then
        west build -b qemu_cortex_max_smp samples/_zvm
    elif [ "$PLAT" = "$plat_fvp" ]; then
        west build -b fvp_cortex_a55 samples/_zvm \
        -DARMFVP_BL1_FILE=/path-to/bl1.bin \
        -DARMFVP_FIP_FILE=/path-to/fip.bin
    elif [ "$PLAT" = "$plat_rk3568" ]; then
        west build -b roc_rk3568_pc samples/_zvm 
    else
        echo "Error arguments for this auto.sh! \n Please input command like: ./auto_build.sh build qemu. "
    fi

elif [ "$OPS" = "$ops_debug" ]; then

    if [ "$PLAT" = "$plat_qemu" ]; then

        /path-to/qemu-system-aarch64 \
        -cpu max -m 4G -nographic -machine virt,virtualization=on,gic-version=3 \
        -net none -pidfile qemu.pid -chardev stdio,id=con,mux=on \
        -serial chardev:con -mon chardev=con,mode=readline -serial pty -serial pty -smp cpus=4 \
        -device loader,file=/path-to/zephyr.bin,addr=0x48000000,force-raw=on \
        -device loader,file=/path-to/Image,addr=0x80000000,force-raw=on \
        -device loader,file=/path-to/linux-qemu-virt.dtb,addr=0x88000000 \
        -kernel /path-to/zvm_host.elf -s -S

    elif [ "$PLAT" = "$plat_fvp" ]; then

#        /opt/arm/developmentstudio-2021.2/bin/FVP_Base_Cortex-A55x4+Cortex-A75x2 	\
        /path-to/FVP_Base_RevC-2xAEMvA \
        -C pctl.startup=0.0.0.* \
        -C bp.secure_memory=0 \
        -C cache_state_modelled=0 \
        -C bp.pl011_uart0.untimed_fifos=1 -C bp.pl011_uart0.unbuffered_output=1 -C bp.pl011_uart0.out_file=uart0.log \
        -C bp.pl011_uart1.out_file=uart1.log \
        -C bp.terminal_0.terminal_command=/usr/bin/gnome-terminal -C bp.terminal_0.mode=raw \
        -C bp.terminal_1.terminal_command=/usr/bin/gnome-terminal -C bp.terminal_1.mode=raw \
        -C bp.secureflashloader.fname=/path-to/bl1.bin \
        -C bp.flashloader0.fname=/path-to/fip.bin \
        -C bp.ve_sysregs.mmbSiteDefault=0 -C bp.ve_sysregs.exit_on_shutdown=1 \
        --data cluster0.cpu0=/path-to/zephyr.bin@0xa0000000 \
        --data cluster0.cpu0=/path-to/Image@0xb0000000 \
        --data cluster0.cpu0=/path-to/fvp-base-gicv3-psci.dtb@0xc0000000   \
        --cpulimit 4 \
        --iris-server

    else
        echo "Error arguments for this auto.sh! \n Please input command like: ./z_auto.sh build qemu. "
    fi

fi 