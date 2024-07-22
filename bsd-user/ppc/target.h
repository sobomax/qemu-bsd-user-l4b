/*
 * PowerPC general target stuff that's common to all aarch details
 *
 * Copyright (c) 2022 M. Warner Losh <imp@bsdimp.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TARGET_H
#define TARGET_H

/*
 * PowerPC64* ABI does not 'lump' the registers for 64-bit args. 32-bit
 * does.
 */
static inline bool regpairs_aligned(void *cpu_env)
{
#if TARGET_ABI_BITS == 32
    return true;
#else
    return false;
#endif
}

#endif /* TARGET_H */

