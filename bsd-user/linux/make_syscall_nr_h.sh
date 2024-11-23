#! /bin/sh -

#
# Usage: 'sh ./make_syscall_nr_h.sh [full path to syscall.h] [syscall_nr.h]'
#

#default input file:
syshdr="/usr/include/sys/syscall.h"

#default output file:
sysnr="./syscall_nr.h"

if [ -n "$1" ]; then
	syshdr=$1
fi

if [ -n "$2" ]; then
	sysnr=$2
fi

echo "/*" > $sysnr
echo " * This file was generated from $syshdr" >> $sysnr
echo " */" >> $sysnr
echo "" >> $sysnr

/usr/bin/sed -e 's:SYS_:TARGET_FREEBSD_NR_:' < $syshdr >> $sysnr
