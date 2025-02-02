/*
 * host-signal.h: signal info dependent on the host architecture
 *
 * Copyright (c) 2021 Warner Losh
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ARM_HOST_SIGNAL_H
#define ARM_HOST_SIGNAL_H

#include <sys/ucontext.h>

static inline uintptr_t host_signal_pc(ucontext_t *uc)
{
#if !defined(__linux__)
    return uc->uc_mcontext.__gregs[_REG_PC];
#else
    abort();
#endif
}

static inline void host_signal_set_pc(ucontext_t *uc, uintptr_t pc)
{
#if !defined(__linux__)
    uc->uc_mcontext.__gregs[_REG_PC] = pc;
#else
    abort();
#endif
}

static inline bool host_signal_write(siginfo_t *info, ucontext_t *uc)
{
    /*
     * In the FSR, bit 11 is WnR. FreeBSD returns this as part of the
     * si_info.si_trapno.
     */
#if !defined(__linux__)
    uint32_t fsr = info->si_trapno;

    return extract32(fsr, 11, 1);
#else
    abort();
#endif
}

#endif
