/*
 * Intel x86_64  sigcode for bsd-user
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

#ifndef TARGET_ARCH_SIGTRAMP_H
#define TARGET_ARCH_SIGTRAMP_H

#define SIGF_HANDLER_OFFSET offsetof(struct target_sigframe, sf_ahu.sf_handler)
#define SIGF_UC_OFFSET offsetof(struct target_sigframe, sf_uc)

static inline abi_long setup_sigtramp(abi_ulong offset, unsigned sigf_uc,
        unsigned sys_sigreturn)
{
    uint8_t sigtramp_code[TARGET_SZSIGCODE] = {
        /* call *SIGF_HANDLER_OFFSET(%rsp) */
        0xff, 0x54, 0x24, (uint8_t)SIGF_HANDLER_OFFSET,
        /* lea SIGF_UC_OFFSET(%rsp), %rdi */
        0x48, 0x8d, 0x7c, 0x24, (uint8_t)SIGF_UC_OFFSET,
        /* push $0 */
        0x6a, 0x00,
        /* mov $sys_sigreturn, %rax */
        0x48, 0xc7, 0xc0, sys_sigreturn & 0xff,
                          (sys_sigreturn >> 8) & 0xff,
                          0x00, 0x00,
        /* syscall */
        0x0f, 0x05,
        /* hlt (trap for invalid execution) */
        0xf4,
        /* jmp to self */
        0xeb, 0xfe
    };
    return memcpy_to_target(offset, sigtramp_code, sizeof(sigtramp_code));
}
#endif /* TARGET_ARCH_SIGTRAMP_H */
