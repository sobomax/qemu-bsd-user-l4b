/*
 *  x86_64 thread support
 *
 *  Copyright (c) 2013 Stacey D. Son
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TARGET_ARCH_THREAD_H
#define TARGET_ARCH_THREAD_H

/* Compare to vm_machdep.c cpu_set_upcall_kse() */
static inline void target_thread_set_upcall(CPUX86State *env, abi_ulong entry,
    abi_ulong arg, abi_ulong stack_base, abi_ulong stack_size)
{
    env->regs[R_EBP] = 0;
    env->regs[R_ESP] = ((stack_base + stack_size) & ~0x0f) - 8;
    env->eip = entry;
    env->regs[R_EDI] = arg;
}

static inline void target_thread_init(struct target_pt_regs *regs,
    struct image_info *infop)
{
    regs->rax = 0;
    regs->rsp = ((infop->start_stack - 8) & ~0xfUL) + 8;
    regs->rip = infop->entry;
    regs->rdi = infop->start_stack;
}

#endif /* TARGET_ARCH_THREAD_H */
