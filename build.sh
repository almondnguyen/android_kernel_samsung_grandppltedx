#!/bin/bash
##
#  Copyright (C) 2015, Samsung Electronics, Co., Ltd.
#  Written by System S/W Group, S/W Platform R&D Team,
#  Mobile Communication Division.
#
#  Edited by Diep Quynh Nguyen (diepquynh1501)
##

set -e -o pipefail

DEFCONFIG=mt6737t-grandpplte_defconfig
NAME=RZ_kernel
VERSION=v1.0

export ARCH=arm
export LOCALVERSION=-${VERSION}

KERNEL_PATH=$(pwd)
KERNEL_ZIP=${KERNEL_PATH}/kernel_zip
KERNEL_ZIP_NAME=${NAME}_${VERSION}.zip
KERNEL_IMAGE=${KERNEL_ZIP}/tools/zImage
DT_IMG=${KERNEL_ZIP}/tools/dt.img
MODULES_PATH=${KERNEL_ZIP}/system/lib/modules
OUTPUT_PATH=${KERNEL_PATH}/output

JOBS=`grep processor /proc/cpuinfo | wc -l`

# Colors
cyan='\033[0;36m'
yellow='\033[0;33m'
red='\033[0;31m'
nocol='\033[0m'

function build() {
	clear;

	BUILD_START=$(date +"%s");
	echo -e "$cyan"
	echo "***********************************************";
	echo "              Compiling RZ kernel          	     ";
	echo -e "***********************************************$nocol";
	echo -e "$red";

	if [ ! -e ${OUTPUT_PATH} ]; then
		mkdir ${OUTPUT_PATH};
	fi;

	if [ ! -e ${MODULES_PATH} ]; then
		mkdir -p ${MODULES_PATH};
	fi;

	echo -e "Initializing defconfig...$nocol";
	make O=output ${DEFCONFIG}
	echo -e "$red";
	echo -e "Building kernel...$nocol";
	make O=output -j${JOBS};
	#make O=output -j${JOBS} dtbs;
	#gcc -o ${KERNEL_PATH}/scripts/dtbTool ${KERNEL_PATH}/scripts/dtbtool.c
	#./scripts/dtbTool -o ${DT_IMG} -s 2048 $(pwd)/output/arch/arm/boot/
	find ${KERNEL_PATH} -name "zImage" -exec mv -f {} ${KERNEL_ZIP}/tools \;
	find ${KERNEL_PATH} -name "*.ko" -exec mv -f {} ${MODULES_PATH} \;

	BUILD_END=$(date +"%s");
	DIFF=$(($BUILD_END - $BUILD_START));
	echo -e "$yellow";
	echo -e "Build completed in $(($DIFF / 60)) minute(s) and $(($DIFF % 60)) seconds.$nocol";
}

function make_zip() {
	echo -e "$red";
	echo -e "Making flashable zip...$nocol";

	cd ${KERNEL_PATH}/kernel_zip;
	zip -r ${KERNEL_ZIP_NAME} ./;
	mv ${KERNEL_ZIP_NAME} ${KERNEL_PATH};
}

function rm_if_exist() {
	if [ -e $1 ]; then
		rm -rf $1;
	fi;
}

function clean() {
	echo -e "$red";
	echo -e "Cleaning build environment...$nocol";
	make -j${JOBS} mrproper;

	rm_if_exist ${KERNEL_ZIP_NAME};
	rm_if_exist ${OUTPUT_PATH};
	rm_if_exist ${MODULES_PATH};
	rm_if_exist ${DT_IMG};
	rm_if_exist ${KERNEL_PATH}/scripts/dtbTool;

	echo -e "$yellow";
	echo -e "Done!$nocol";
}

function main() {
	clear;
	read -p "Please specify Toolchain path: " tcpath;
	if [ "${tcpath}" == "" ]; then
		echo -e "$red"
		export CROSS_COMPILE=$(pwd)/linaro-6.2/bin/arm-eabi-;
		echo -e "No toolchain path found. Using default local one:$nocol ${CROSS_COMPILE}";
	else
		export CROSS_COMPILE=${tcpath};
		echo -e "$red";
		echo -e "Specified toolchain path: $nocol ${CROSS_COMPILE}";
	fi;

	echo -e "*****************************************************************************";
	echo "      RZ Kernel for Samsung Galaxy J2 Prime (Grand Prime Plus) SM-G532x";
	echo -e "*****************************************************************************";
	echo "Choices:";
	echo "1. Cleanup source";
	echo "2. Build kernel";
	echo "3. Build kernel then make flashable ZIP";
	echo "4. Make flashable ZIP package";
	echo "Leave empty to exit this script (it'll show invalid choice)";

	read -n 1 -p "Select your choice: " -s choice;
	case ${choice} in
		1) clean;;
		2) build;;
		3) build
		   make_zip;;
		4) make_zip;;
		*) echo
		   echo "Invalid choice entered. Exiting..."
		   sleep 2;
		   exit 1;;
	esac
}

main $@
