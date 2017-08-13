cmd_arch/arm/crypto/sha256-core.o := ~/android/lineage/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-gcc -Wp,-MD,arch/arm/crypto/.sha256-core.o.d  -nostdinc -isystem /home/meg/android/lineage/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/../lib/gcc/arm-eabi/4.8/include -I./arch/arm/include -Iarch/arm/include/generated  -Iinclude -I./arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I./include/uapi -Iinclude/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-mediatek/include -I./drivers/misc/mediatek/include/mt-plat/mt6735/include -I./arch/arm/mach-mediatek/include/  -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -mfpu=vfp -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float -Wa,-gdwarf-2         -c -o arch/arm/crypto/sha256-core.o arch/arm/crypto/sha256-core.S

source_arch/arm/crypto/sha256-core.o := arch/arm/crypto/sha256-core.S

deps_arch/arm/crypto/sha256-core.o := \
  arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \

arch/arm/crypto/sha256-core.o: $(deps_arch/arm/crypto/sha256-core.o)

$(deps_arch/arm/crypto/sha256-core.o):
