#!/sbin/sh
#
# Live ramdisk patching script
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Please maintain this if you use this script or any part of it

# Head to the build tools extracted folder
cd /tmp;

# Get current boot.img first then unpack it
dd if=/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/boot of=/tmp/boot.img;
./unpackbootimg -i /tmp/boot.img;

# Initialize boot.img's properties
base=$(cat *-base);
cmdline=$(cat *-cmdline);
pagesize=$(cat *-pagesize);
ramdisk_offset=$(cat *-ramdisk_offset);
tags_offset=$(cat *-tags_offset);
board="SRPPI01A000KU"

# Pack our new boot.img
./mkbootimg --kernel /tmp/zImage --ramdisk /tmp/boot.img-ramdisk.gz --cmdline "$cmdline" --base $base --pagesize $pagesize --ramdisk_offset $ramdisk_offset --tags_offset $tags_offset --dt /tmp/boot.img-dt --board "$board" -o /tmp/newboot.img;

# Prevent modified kernel warning of newer Samsung devices
echo "SEANDROIDENFORCE" >> /tmp/newboot.img;

# It's flashing time!!
dd if=/tmp/newboot.img of=/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/boot;
