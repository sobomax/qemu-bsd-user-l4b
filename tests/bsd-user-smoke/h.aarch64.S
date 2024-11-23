# Copyright (c) 2021 Warner Losh
# SPDX-License-Identifier: BSD-2-Clause

#if !defined(__linux__)
#include <sys/syscall.h>
#endif
#define STDOUT_FILENO	1

	.text
	.globl	qemu_start
	.p2align	2
qemu_start:

	# write(1, .L.str, sizeof(.L.str) - 1)
	adr	x1, .L.str
	mov	w0, #STDOUT_FILENO
	mov	w2, #len
	mov	x8, #SYS_write
	svc	0

	# _exit(0)
	mov	w0, wzr
	mov	x8, #SYS_exit
	svc	0

	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"Hello World\n"
.L.strEnd:
len=.L.strEnd - .L.str - 1
