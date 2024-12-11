/*
 * host-signal.h: signal info dependent on the host architecture
 *
 * Copyright (c) 2021 Warner Losh
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef AARCH_HOST_SIGNAL_H
#define AARCH_HOST_SIGNAL_H

#include <sys/ucontext.h>

static inline uintptr_t host_signal_pc(ucontext_t *uc)
{
#if !defined(__linux__)
    return uc->uc_mcontext.mc_gpregs.gp_lr;
#else
    abort();
#endif
}

static inline void host_signal_set_pc(ucontext_t *uc, uintptr_t pc)
{
#if !defined(__linux__)
    uc->uc_mcontext.mc_gpregs.gp_lr = pc;
#else
    abort();
#endif
}

static inline bool host_signal_write(siginfo_t *info, ucontext_t *uc)
{
    uint32_t insn;

    /* Linux provides data to decode this, but FreeBSD does not */

    /*
     * Fall back to parsing instructions; will only be needed
     * for really ancient (pre-3.16) kernels.
     */
    insn = *(uint32_t *)host_signal_pc(uc);

    return (insn & 0xbfff0000) == 0x0c000000   /* C3.3.1 */
        || (insn & 0xbfe00000) == 0x0c800000   /* C3.3.2 */
        || (insn & 0xbfdf0000) == 0x0d000000   /* C3.3.3 */
        || (insn & 0xbfc00000) == 0x0d800000   /* C3.3.4 */
        || (insn & 0x3f400000) == 0x08000000   /* C3.3.6 */
        || (insn & 0x3bc00000) == 0x39000000   /* C3.3.13 */
        || (insn & 0x3fc00000) == 0x3d800000   /* ... 128bit */
        /* Ignore bits 10, 11 & 21, controlling indexing.  */
        || (insn & 0x3bc00000) == 0x38000000   /* C3.3.8-12 */
        || (insn & 0x3fe00000) == 0x3c800000   /* ... 128bit */
        /* Ignore bits 23 & 24, controlling indexing.  */
        || (insn & 0x3a400000) == 0x28000000; /* C3.3.7,14-16 */

}

#endif
