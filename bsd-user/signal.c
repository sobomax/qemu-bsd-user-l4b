/*
 *  Emulation of BSD signals
 *
 *  Copyright (c) 2003 - 2008 Fabrice Bellard
 *  Copyright (c) 2013 Stacey Son
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
#include "signal-common.h"
#include "trace.h"
#include "hw/core/tcg-cpu-ops.h"
#include "host-signal.h"

/*
 * Stubbed out routines until we merge signal support from bsd-user
 * fork.
 */

static struct target_sigaction sigact_table[TARGET_NSIG];
static void host_signal_handler(int host_sig, siginfo_t *info, void *puc);

int host_to_target_signal(int sig)
{
    return sig;
}

int target_to_host_signal(int sig)
{
    return sig;
}

/* Siginfo conversion. */
static inline void host_to_target_siginfo_noswap(target_siginfo_t *tinfo,
        const siginfo_t *info)
{
    int sig, code;

    sig = host_to_target_signal(info->si_signo);
    /* XXX should have host_to_target_si_code() */
    code = tswap32(info->si_code);
    tinfo->si_signo = sig;
    tinfo->si_errno = info->si_errno;
    tinfo->si_code = info->si_code;
    tinfo->si_pid = info->si_pid;
    tinfo->si_uid = info->si_uid;
    tinfo->si_status = info->si_status;
    tinfo->si_addr = (abi_ulong)(unsigned long)info->si_addr;
    /* si_value is opaque to kernel */
    tinfo->si_value.sival_ptr =
        (abi_ulong)(unsigned long)info->si_value.sival_ptr;
    if (SIGILL == sig || SIGFPE == sig || SIGSEGV == sig || SIGBUS == sig ||
            SIGTRAP == sig) {
        tinfo->_reason._fault._trapno = info->_reason._fault._trapno;
    }
#ifdef SIGPOLL
    if (SIGPOLL == sig) {
        tinfo->_reason._poll._band = info->_reason._poll._band;
    }
#endif
    if (SI_TIMER == code) {
        int timerid;

        timerid = info->_reason._timer._timerid;
        tinfo->_reason._timer._timerid = timerid;
        tinfo->_reason._timer._overrun = info->_reason._timer._overrun;
    }
}

/*
 * Queue a signal so that it will be send to the virtual CPU as soon as
 * possible.
 */
void queue_signal(CPUArchState *env, int sig, target_siginfo_t *info)
{
    qemu_log_mask(LOG_UNIMP, "No signal queueing, dropping signal %d\n", sig);
}

static int fatal_signal(int sig)
{

    switch (sig) {
    case TARGET_SIGCHLD:
    case TARGET_SIGURG:
    case TARGET_SIGWINCH:
    case TARGET_SIGINFO:
        /* Ignored by default. */
        return 0;
    case TARGET_SIGCONT:
    case TARGET_SIGSTOP:
    case TARGET_SIGTSTP:
    case TARGET_SIGTTIN:
    case TARGET_SIGTTOU:
        /* Job control signals.  */
        return 0;
    default:
        return 1;
    }
}

/*
 * Force a synchronously taken QEMU_SI_FAULT signal. For QEMU the
 * 'force' part is handled in process_pending_signals().
 */
void force_sig_fault(int sig, int code, abi_ulong addr)
{
    CPUState *cpu = thread_cpu;
    CPUArchState *env = cpu->env_ptr;
    target_siginfo_t info = {};

    info.si_signo = sig;
    info.si_errno = 0;
    info.si_code = code;
    info.si_addr = addr;
    queue_signal(env, sig, &info);
}

static void host_signal_handler(int host_sig, siginfo_t *info, void *puc)
{
}

void signal_init(void)
{
    TaskState *ts = (TaskState *)thread_cpu->opaque;
    struct sigaction act;
    struct sigaction oact;
    int i;
    int host_sig;

    /* Set the signal mask from the host mask. */
    sigprocmask(0, 0, &ts->signal_mask);

    /*
     * Set all host signal handlers. ALL signals are blocked during the
     * handlers to serialize them.
     */
    memset(sigact_table, 0, sizeof(sigact_table));

    sigfillset(&act.sa_mask);
    act.sa_sigaction = host_signal_handler;
    act.sa_flags = SA_SIGINFO;

    for (i = 1; i <= TARGET_NSIG; i++) {
        host_sig = target_to_host_signal(i);
        sigaction(host_sig, NULL, &oact);
        if (oact.sa_sigaction == (void *)SIG_IGN) {
            sigact_table[i - 1]._sa_handler = TARGET_SIG_IGN;
        } else if (oact.sa_sigaction == (void *)SIG_DFL) {
            sigact_table[i - 1]._sa_handler = TARGET_SIG_DFL;
        }
        /*
         * If there's already a handler installed then something has
         * gone horribly wrong, so don't even try to handle that case.
         * Install some handlers for our own use.  We need at least
         * SIGSEGV and SIGBUS, to detect exceptions.  We can not just
         * trap all signals because it affects syscall interrupt
         * behavior.  But do trap all default-fatal signals.
         */
        if (fatal_signal(i)) {
            sigaction(host_sig, &act, NULL);
        }
    }
}

void process_pending_signals(CPUArchState *cpu_env)
{
}

void cpu_loop_exit_sigsegv(CPUState *cpu, target_ulong addr,
                           MMUAccessType access_type, bool maperr, uintptr_t ra)
{
    const struct TCGCPUOps *tcg_ops = CPU_GET_CLASS(cpu)->tcg_ops;

    if (tcg_ops->record_sigsegv) {
        tcg_ops->record_sigsegv(cpu, addr, access_type, maperr, ra);
    }

    force_sig_fault(TARGET_SIGSEGV,
                    maperr ? TARGET_SEGV_MAPERR : TARGET_SEGV_ACCERR,
                    addr);
    cpu->exception_index = EXCP_INTERRUPT;
    cpu_loop_exit_restore(cpu, ra);
}

void cpu_loop_exit_sigbus(CPUState *cpu, target_ulong addr,
                          MMUAccessType access_type, uintptr_t ra)
{
    const struct TCGCPUOps *tcg_ops = CPU_GET_CLASS(cpu)->tcg_ops;

    if (tcg_ops->record_sigbus) {
        tcg_ops->record_sigbus(cpu, addr, access_type, ra);
    }

    force_sig_fault(TARGET_SIGBUS, TARGET_BUS_ADRALN, addr);
    cpu->exception_index = EXCP_INTERRUPT;
    cpu_loop_exit_restore(cpu, ra);
}
