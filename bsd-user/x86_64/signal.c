/*
 *  x86_64 signal definitions
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

#include "qemu/osdep.h"
#include "qemu.h"

#define TARGET_PSL_C           0x00000001      /* carry bit */
#define TARGET_PSL_T           0x00000100      /* trace enable bit */
#define TARGET_PSL_D           0x00000400      /* string instruction direction bit */

/*
 * Compare to amd64/amd64/machdep.c sendsig()
 * Assumes that target stack frame memory is locked.
 */
abi_long set_sigtramp_args(CPUX86State *env,
        int sig, struct target_sigframe *frame, abi_ulong frame_addr,
        struct target_sigaction *ka)
{
    target_mcontext_t *mcp = &frame->sf_uc.uc_mcontext;

    env->regs[R_EDI] = sig;
    env->regs[R_EDX] = frame_addr +
        offsetof(struct target_sigframe, sf_uc);
    env->regs[R_ESI] = frame_addr +
        offsetof(struct target_sigframe, sf_si);
    env->regs[R_ECX] = mcp->mc_rip;
    env->regs[R_ESP] = frame_addr;
    env->eip = TARGET_PS_STRINGS - TARGET_SZSIGCODE;
    frame->sf_ahu.sf_action = ka->_sa_handler;
    env->eflags &= ~(TARGET_PSL_T | TARGET_PSL_D);
    return 0;
}

/*
 * Compare to amd64/amd64/exec_machdep.c sendsig()
 * Assumes that the memory is locked if frame points to user memory.
 */
abi_long setup_sigframe_arch(CPUX86State *env, abi_ulong frame_addr,
                             struct target_sigframe *frame, int flags)
{
    target_mcontext_t *mcp = &frame->sf_uc.uc_mcontext;

    get_mcontext(env, mcp, flags);
    return 0;
}

static void get_fpcontext(CPUX86State *env, target_mcontext_t *mcp, char **xfpusave, size_t *xfpusave_len) {
    /* Mark FPU ownership */
    mcp->mc_ownedfp = (env->fp_status.float_exception_flags != 0);

    /* Copy the FPU state */
    static_assert(sizeof(mcp->mc_fpstate) >= sizeof(env->fpregs), "sizeof(mcp->mc_fpstate) < sizeof(env->fpregs)");
    memcpy(&mcp->mc_fpstate[0], &env->fpregs, sizeof(env->fpregs));

    /* Indicate FP format */
    mcp->mc_fpformat = 0;

    /* Handle extended state (XSAVE) if supported */
    if (xfpusave == NULL) {
        return;
    }
    abort();
}

#define TARGET_GET_MC_CLEAR_RET 1
#define TARGET_TF_HASSEGS      0x1

/* Compare to amd64/amd64/machdep.c get_mcontext() */
abi_long get_mcontext(CPUX86State *env,
                target_mcontext_t *mcp, int flags)
{
    mcp->mc_onstack = 1; /* XXX */
    mcp->mc_r15 = env->regs[R_R15];
    mcp->mc_r14 = env->regs[R_R14];
    mcp->mc_r13 = env->regs[R_R13];
    mcp->mc_r12 = env->regs[R_R12];
    mcp->mc_r11 = env->regs[R_R11];
    mcp->mc_r10 = env->regs[R_R10];
    mcp->mc_r9 = env->regs[R_R9];
    mcp->mc_r8 = env->regs[R_R8];
    mcp->mc_rdi = env->regs[R_EDI];
    mcp->mc_rsi = env->regs[R_ESI];
    mcp->mc_rbp = env->regs[R_EBP];
    mcp->mc_rbx = env->regs[R_EBX];
    mcp->mc_rcx = env->regs[R_ECX];
    mcp->mc_rflags = env->eflags;
    if (flags & TARGET_GET_MC_CLEAR_RET) {
        mcp->mc_rax = 0;
        mcp->mc_rdx = 0;
        mcp->mc_rflags &= ~TARGET_PSL_C;
    } else {
        mcp->mc_rax = env->regs[R_EAX];
        mcp->mc_rdx = env->regs[R_EDX];
    }
    mcp->mc_rip = env->eip;
    mcp->mc_cs = env->segs[R_CS].selector;
    mcp->mc_rsp = env->regs[R_ESP];
    mcp->mc_ss = env->segs[R_SS].selector;
    mcp->mc_ds = env->segs[R_DS].selector;
    mcp->mc_es = env->segs[R_ES].selector;
    mcp->mc_fs = env->segs[R_FS].selector;
    mcp->mc_gs = env->segs[R_GS].selector;
    mcp->mc_flags = TARGET_TF_HASSEGS;
    mcp->mc_len = sizeof(*mcp);
    get_fpcontext(env, mcp, NULL, NULL);
    mcp->mc_fsbase = env->segs[R_FS].base;
    mcp->mc_gsbase = env->segs[R_GS].base;
    mcp->mc_xfpustate = 0;
    mcp->mc_xfpustate_len = 0;
    bzero(mcp->mc_spare, sizeof(mcp->mc_spare));
    return 0;
}

static void set_fpcontext(CPUX86State *env, const target_mcontext_t *mcp, const char *xfpusave, size_t xfpusave_len) {
    /* Restore FPU ownership status */
    if (mcp->mc_ownedfp) {
        env->fp_status.float_exception_flags = 1; /* Indicate FPU is in use */
    } else {
        env->fp_status.float_exception_flags = 0; /* No FPU activity */
    }

    /* Restore the FPU state */
    static_assert(sizeof(env->fpregs) <= sizeof(mcp->mc_fpstate),
                  "sizeof(env->fpregs) > sizeof(mcp->mc_fpstate)");
    memcpy(&env->fpregs, &mcp->mc_fpstate[0], sizeof(env->fpregs));

    if (mcp->mc_fpformat != 0) {
        /* Unsupported FP format, abort for now */
        abort();
    }

    if (xfpusave != NULL || xfpusave_len > 0) {
        abort();
    }
}

/* Compare to amd64/amd64/machdep.c set_mcontext() */
abi_long set_mcontext(CPUX86State *env, target_mcontext_t *mcp, int srflag)
{
    env->regs[R_R15] = mcp->mc_r15;
    env->regs[R_R14] = mcp->mc_r14;
    env->regs[R_R13] = mcp->mc_r13;
    env->regs[R_R12] = mcp->mc_r12;
    env->regs[R_R11] = mcp->mc_r11;
    env->regs[R_R10] = mcp->mc_r10;
    env->regs[R_R9]  = mcp->mc_r9;
    env->regs[R_R8]  = mcp->mc_r8;
    env->regs[R_EDI] = mcp->mc_rdi;
    env->regs[R_ESI] = mcp->mc_rsi;
    env->regs[R_EBP] = mcp->mc_rbp;
    env->regs[R_EBX] = mcp->mc_rbx;
    env->regs[R_ECX] = mcp->mc_rcx;
    env->regs[R_EAX] = mcp->mc_rax;
    env->regs[R_EDX] = mcp->mc_rdx;

    env->eflags = mcp->mc_rflags;

    /* Set the instruction pointer and stack pointer */
    env->eip = mcp->mc_rip;
    env->regs[R_ESP] = mcp->mc_rsp;

    /* Restore segment selectors */
    env->segs[R_CS].selector = mcp->mc_cs;
    env->segs[R_SS].selector = mcp->mc_ss;
    env->segs[R_DS].selector = mcp->mc_ds;
    env->segs[R_ES].selector = mcp->mc_es;
    env->segs[R_FS].selector = mcp->mc_fs;
    env->segs[R_GS].selector = mcp->mc_gs;

    /* Restore segment bases if present */
    env->segs[R_FS].base = mcp->mc_fsbase;
    env->segs[R_GS].base = mcp->mc_gsbase;

    set_fpcontext(env, mcp, NULL, 0);

    /* Handle extended FP state if necessary */
    if (mcp->mc_xfpustate && mcp->mc_xfpustate_len > 0) {
        /* Restore FP/XMM state - placeholder for actual FP logic */
        /* Example: load_fpcontext(env, mcp->mc_xfpustate, mcp->mc_xfpustate_len); */
    }

    return 0;
}

abi_long get_ucontext_sigreturn(CPUX86State *regs,
        abi_ulong target_sf, abi_ulong *target_uc)
{
    *target_uc = target_sf;
    return 0;
}
