#!/bin/bash
export CROSS_COMPILE=../PLATFORM/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-
export ARCH=arm

make mt6737t-grandpplte_defconfig
make -j