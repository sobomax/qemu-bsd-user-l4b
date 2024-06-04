# Copyright (c) 2021 Warner Losh
# SPDX-License-Identifier: BSD-2-Clause

#include <sys/syscall.h>
#define STDOUT_FILENO	1

	.text
	.attribute	4, 16
	.globl	qemu_start
	.p2align	1
qemu_start:

	# write(1, .L.str, sizeof(.L.str) - 1)
	lui	a0, %hi(.L.str)
	addi	a1, a0, %lo(.L.str)
	addi	a0, zero, STDOUT_FILENO
	addi	a2, zero, 12
	li	t0, SYS_write
	ecall

	# _exit(0)
	li	a0, 0
	li	t0, SYS_exit
	ecall
	ret

	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"Hello World\n"
.L.strEnd:
len=.L.strEnd - .L.str - 1
