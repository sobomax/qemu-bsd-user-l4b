/*
 *  powerpc signal definitions
 *
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

#ifndef TARGET_ARCH_SIGNAL_H
#define TARGET_ARCH_SIGNAL_H

#include "cpu.h"

extern bool bsd_ppc_is_elfv1(CPUPPCState *env);

#define TARGET_INSN_SIZE     4  /* powerpc instruction size */

#if defined(TARGET_PPC64) && !defined(TARGET_ABI32)
/* Size of the signal trampoline code placed on the stack. */
#define TARGET_SZSIGCODE    ((abi_ulong)(12 * TARGET_INSN_SIZE))
#else
#define TARGET_SZSIGCODE    ((abi_ulong)(8 * TARGET_INSN_SIZE))
#endif

#define TARGET_MINSIGSTKSZ  (512 * 4)
#define TARGET_SIGSTKSZ     (TARGET_MINSIGSTKSZ + 32768)

/* compare to sys/powerpc/include/frame.h */
#if defined(TARGET_PPC64) && !defined(TARGET_ABI32)
#define TARGET_SZREG        8
#define TARGET_CALLFRAME_SIZ    (TARGET_SZREG * 10)
#else
#define TARGET_SZREG        4
#define TARGET_CALLFRAME_SIZ    (TARGET_SZREG * 6)
#endif

typedef struct target_mcontext {
	abi_int     mc_vers;
	abi_int     mc_flags;
#define TARGET_MC_FP_VALID		0x0001
#define TARGET_MC_AV_VALID		0x0002
	abi_int     mc_onstack;     /* sigstack state to restore */
	abi_int     mc_len;
	uint64_t    mc_avec[32*2];
	uint32_t    mc_av[2];
	abi_long    mc_frame[42];    /* process regs 0 to 31 */
	uint64_t    mc_fpreg[33];  /* fp regs 0 to 31  */
	uint64_t    mc_vsxfpreg[32]; /* low-order half of VSR0-31 */
} target_mcontext_t __aligned(16);

#if defined(TARGET_PPC64) && !defined(TARGET_ABI32)
#define TARGET_MCONTEXT_SIZE 1392
#define TARGET_UCONTEXT_SIZE 1472
#else
#define TARGET_MCONTEXT_SIZE 1224
#define TARGET_UCONTEXT_SIZE 1280
#endif

#include "target_os_ucontext.h"

struct target_sigframe {
	target_ucontext_t sf_uc;
	target_siginfo_t  sf_si;
};

#define TARGET_SIGSTACK_ALIGN 16

#endif /* TARGET_ARCH_SIGNAL_H */
