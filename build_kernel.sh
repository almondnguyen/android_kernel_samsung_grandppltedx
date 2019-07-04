#!/bin/bash
#
# Copyright (C) 2018 Yuvraj Saxena (frap130)
# Copyright (C) 2018 SaMad SegMane (MrSaMaDo)
#
 
# Script To Compile Kernels
 
# Colors
ORNG=$'\033[0;33m'
CYN=$'\033[0;36m'
PURP=$'\033[0;35m'
BLINK_RED=$'\033[05;31m'
BLUE=$'\033[01;34m'
BLD=$'\033[1m'
GRN=$'\033[01;32m'
RED=$'\033[01;31m'
RST=$'\033[0m'
YLW=$'\033[01;33m'
 
function START_SCRIPT() {
echo "${GRN}
 
 __      __       .__                                ___________      ___________.__             __________      .__.__       .___   _________            .__        __   
/  \    /  \ ____ |  |   ____  ____   _____   ____   \__    ___/___   \__    ___/|  |__   ____   \______   \__ __|__|  |    __| _/  /   _____/ ___________|__|______/  |_ 
\   \/\/   // __ \|  | _/ ___\/  _ \ /     \_/ __ \    |    | /  _ \    |    |   |  |  \_/ __ \   |    |  _/  |  \  |  |   / __ |   \_____  \_/ ___\_  __ \  \____ \   __\
 \        /\  ___/|  |_\  \__(  <_> )  Y Y  \  ___/    |    |(  <_> )   |    |   |   Y  \  ___/   |    |   \  |  /  |  |__/ /_/ |   /        \  \___|  | \/  |  |_> >  |  
  \__/\  /  \___  >____/\___  >____/|__|_|  /\___  >   |____| \____/    |____|   |___|  /\___  >  |______  /____/|__|____/\____ |  /_______  /\___  >__|  |__|   __/|__|  
       \/       \/          \/            \/     \/                                   \/     \/          \/                    \/          \/     \/         |__|         
 
 ${RST}" 
        HELP
        read -rsn1 answer
	case "${answer}" in
                "b"|"build")
                        BUILD=y
                        ;;
		"c"|"clean")
			CLEAN=y
			;;
		*)			
                        echo "${RED}Opps:Invalid Option${RST}"
                        exit 1;
			;;
		esac
		shift
}
 
function BANNER() {
echo "${BLINK_RED}        
 
▀█████████▄   ▄█          ▄████████  ▄████████    ▄█   ▄█▄    ▄██████▄     ▄█    █▄     ▄██████▄     ▄████████     ███     
  ███    ███ ███         ███    ███ ███    ███   ███ ▄███▀   ███    ███   ███    ███   ███    ███   ███    ███ ▀█████████▄ 
  ███    ███ ███         ███    ███ ███    █▀    ███▐██▀     ███    █▀    ███    ███   ███    ███   ███    █▀     ▀███▀▀██ 
 ▄███▄▄▄██▀  ███         ███    ███ ███         ▄█████▀     ▄███         ▄███▄▄▄▄███▄▄ ███    ███   ███            ███   ▀ 
▀▀███▀▀▀██▄  ███       ▀███████████ ███        ▀▀█████▄    ▀▀███ ████▄  ▀▀███▀▀▀▀███▀  ███    ███ ▀███████████     ███     
  ███    ██▄ ███         ███    ███ ███    █▄    ███▐██▄     ███    ███   ███    ███   ███    ███          ███     ███     
  ███    ███ ███▌    ▄   ███    ███ ███    ███   ███ ▀███▄   ███    ███   ███    ███   ███    ███    ▄█    ███     ███     
▄█████████▀  █████▄▄██   ███    █▀  ████████▀    ███   ▀█▀   ████████▀    ███    █▀     ▀██████▀   ▄████████▀     ▄████▀   
             ▀                                   ▀                                             
${RST} "
}
 
function BINFO() {
        VERSION="V3"
        BUILD_DATE=$(echo "$(date)" | sed 'y/ /-/')
        export KBUILD_BUILD_USER=Melek
        export KBUILD_BUILD_HOST=superkernel
}
 
function TOOLCHAIN() {
if [[ ! -d gcc ]]; then 
    sudo rm -rf linaro
#   sudo rm -rf gcc/.git
    echo "${GRN} #####################################"
    echo "${GRN} #                                   #"
    echo "${GRN} #        Cloning TOOLCHAIN...       #"
    echo "${GRN} #                                   #"
    echo "${GRN} #####################################"
    git clone -q https://github.com/Skyrimus/arm-eabi-4.8-google-mt6580
    export ARCH=arm CROSS_COMPILE=${PWD}/arm-eabi-4.8-google-mt6580/bin/arm-eabi-
    export SUBARCH=arm
else
    export ARCH=arm CROSS_COMPILE=${PWD}/arm-eabi-4.8-google-mt6580/bin/arm-eabi-
    export SUBARCH=arm
fi
}
 
function BUILD() {
        mkdir -p out
        echo "${PURP} #####################################"
        echo "${PURP} #                                   #"
        echo "${PURP} #        READING DEFCONFIG          #"
        echo "${PURP} #                                   #"
        echo "${PURP} #####################################"
        make O=BGH-Out/ TARGET_ARCH=arm mt6737t-grandpplte_defconfig | tee -a defconfig.log
        echo "${YLW} #####################################"
        echo "${YLW} #                                   #"
        echo "${YLW} #       REAL BUILDING STARTED       #"
        echo "${YLW} #                                   #"
        echo "${YLW} #####################################"
        make -j$(nproc --all) O=BGH-Out/ TARGET_ARCH=arm | tee -a kernel.log
        OIMAGE=BGH-Out/arch/arm/boot/zImage-dtb
}
 
function CHECK() {
if [[ ! -e ${OIMAGE} ]];
then
        echo "${RED} #####################################"
        echo "${RED} #                                   #"
        echo "${RED} #           BUILD ERROR             #"
        echo "${RED} #                                   #"
        echo "${RED} #####################################"
        TRANSFER kernel.log
else
        BANNER
        echo "${GRN} #####################################"
        echo "${GRN} #                                   #"
        echo "${GRN} #    SUCCESSFULLY BUILDED KERNEL    #"
        echo "${GRN} #                                   #"
        echo "${GRN} #####################################"
        mv ${OIMAGE} zImage
        zip -r9 BlackGHostKernel-${VERSION}-${BUILD_DATE}.zip zImage >> /dev/null
	rm zImage
        echo " "
        echo "${GRN} #      Do You Want To Upload?       #"
        echo " "
        read -rsn1 option
        [ "$option" == "y" ] && TRANSFER BlackGHostKernel-${VERSION}-${BUILD_DATE}.zip
fi
}
 
function CLEAN() {
        echo "${GRN} #####################################"
        echo "${GRN} #                                   #"
        echo "${GRN} #     Cleaning Kernel Source..      #"
        echo "${GRN} #                                   #"
        echo "${GRN} #####################################"
	rm -rf *.log *.zip out BGH-Out
        make clean >> /dev/null
        make mrproper >> /dev/null
clear
}
 
function HELP() {
        echo "${GRN} #####################################"
        echo "${GRN} #  BlackGHost Kernel Build Script   #"
        echo "${GRN} #                                   #"
        echo "${GRN} #  b, build  >   Build The Kernel   #"
        echo "${GRN} #  c, clean  >   Clean The Source   #"
        echo "${GRN} #                                   #"
        echo "${GRN} #####################################"
}
 
function TRANSFER() {
        file="$1"
        zipname=$(echo "${file}" | awk -F '/' '{print $NF}')
        destination="$2"
        url=$(curl -# -T "${file}" https://transfer.sh/${destination})
        echo "Download $zipname at $url"
}
 
function FORMAT_TIME() {
        MINS=$(((${1}-${2})/60))
        SECS=$(((${1}-${2})%60))
if [[ ${MINS} -ge 60 ]]; then
        HOURS=$((${MINS}/60))
        MINS=$((${MINS}%60))
fi
 
if [[ ${HOURS} -eq 1 ]]; then
        TIME_STRING+="1 HOUR, "
elif [[ ${HOURS} -ge 2 ]]; then
        TIME_STRING+="${HOURS} HOURS, "
fi
 
if [[ ${MINS} -eq 1 ]]; then
        TIME_STRING+="1 MINUTE"
else
        TIME_STRING+="${MINS} MINUTES"
fi
 
if [[ ${SECS} -eq 1 && -n ${HOURS} ]]; then
        TIME_STRING+=", AND 1 SECOND"
elif [[ ${SECS} -eq 1 && -z ${HOURS} ]]; then
        TIME_STRING+=" AND 1 SECOND"
elif [[ ${SECS} -ne 1 && -n ${HOURS} ]]; then
        TIME_STRING+=", AND ${SECS} SECONDS"
elif [[ ${SECS} -ne 1 && -z ${HOURS} ]]; then
        TIME_STRING+=" AND ${SECS} SECONDS"
fi
 
        echo ${TIME_STRING}
}
 
        BANNER
 
if [[ ${BUILDING_ON_CI} = 'y' ]]; then
        START_SCRIPT "${@}"
else
	START_SCRIPT
fi
 
if [[ "${BUILD}" = 'y' ]]; then
clear
        START=$(date +"%s")
        TOOLCHAIN
clear
        BINFO
        sleep 0.3
        BUILD
        CHECK
        END=$(date +%s)
        TIME_STRING="$(FORMAT_TIME "${START}" "${END}")"
        echo "${GRN}Completed In: ${TIME_STRING}"
elif [[ "${CLEAN}" = 'y' ]]; then
        CLEAN
else
if [[ "${HELP}" = 'y' ]]; then
clear
        HELP
fi
fi
fi