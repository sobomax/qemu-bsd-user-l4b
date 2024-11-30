/*
 * Copyright Warner Losh <imp@bsdimp.com>
 *
 * bsd-2-clause
 */

#include "qemu/osdep.h"
#include "qemu.h"
#include "truss_hdr.h"
#include <sys/syscall.h>
#include "systruss.h" /* Generated from sys/syscalls.h */

static void alloc_syscall(TaskState *t, int num)
{
    assert(t->in_syscall == 0);
    assert(t->cs.number == 0);
    assert(t->cs.sc == NULL);
    assert(t->cs.nargs == 0);
    for (u_int i = 0; i < nitems(t->cs.s_args); i++) {
        assert(t->cs.s_args[i] == NULL);
    }
    memset(t->cs.args, 0, sizeof(t->cs.args));
    t->cs.number = num;
    t->cs.sc = &decoded_syscalls[num];
    t->cs.nargs = t->cs.sc->nargs;
    t->outfile = fmemopen(t->trace_buf, sizeof(t->trace_buf), "w");
    t->in_syscall = 1;
}

static void free_syscall(TaskState *t)
{
    for (u_int i = 0; i < t->cs.nargs; i++) {
        free(t->cs.s_args[i]);
    }
    memset(&t->cs, 0, sizeof(t->cs));
    fclose(t->outfile);
    t->outfile = NULL;
    t->in_syscall = 0;
}

static void
print_pointer(FILE *fp, uintptr_t arg)
{

	fprintf(fp, "%p", (void *)arg);
}

/*
 * Converts a syscall argument into a string.  Said string is
 * allocated via malloc(), so needs to be free()'d.  sc is
 * a pointer to the syscall description (see above); args is
 * an array of all of the system call arguments.
 */
static char *print_arg(TaskState *ts, const struct syscall_arg *sc,
                       abi_long *args, abi_ulong ret, abi_ulong ret2)
{
    FILE *fp;
    char *tmp;
    size_t tmplen;

    fp = open_memstream(&tmp, &tmplen);
    switch (sc->type & ARG_MASK) {
    case Octal:
        fprintf(fp, "0%o", (int)args[sc->offset]);
        break;
    case Int:
        fprintf(fp, "%d", (int)args[sc->offset]);
        break;
    case UInt:
        fprintf(fp, "%u", (unsigned int)args[sc->offset]);
        break;
    default:
    case LongHex:
        fprintf(fp, "0x%lx", (long)args[sc->offset]);
        break;
    case Long:
        fprintf(fp, "%ld", (long)args[sc->offset]);
        break;
    case Sizet:
        fprintf(fp, "%zu", (size_t)args[sc->offset]);
        break;
    case Ptr:
        print_pointer(fp, args[sc->offset]);
        break;
    }
    fclose(fp);
    return (tmp);
}

void record_syscall(TaskState *ts, int num, abi_long arg1, abi_long arg2,
                    abi_long arg3, abi_long arg4, abi_long arg5, abi_long arg6,
                    abi_long arg7, abi_long arg8)
{
    u_int narg, i;

    alloc_syscall(ts, num);
    narg = MIN(ts->cs.nargs, nitems(ts->cs.args));
    i = 0;
    ts->cs.args[i++] = arg1;
    ts->cs.args[i++] = arg2;
    ts->cs.args[i++] = arg3;
    ts->cs.args[i++] = arg4;
    ts->cs.args[i++] = arg5;
    ts->cs.args[i++] = arg6;
    ts->cs.args[i++] = arg7;
    ts->cs.args[i++] = arg8;
    for (i = 0; i < narg; i++) {
        if (!(ts->cs.sc->args[i].type & OUT)) {
            ts->cs.s_args[i] = print_arg(ts, &ts->cs.sc->args[i], ts->cs.args, 0, 0);
        }
    }
}

static void print_syscall_ret(TaskState *ts, abi_ulong ret, abi_ulong ret2, int error)
{
    const char *name;
    char **s_args;
    int i, len, nargs;

    name = ts->cs.sc->name;
    nargs = ts->cs.nargs;
    s_args = ts->cs.s_args;
    
    len = fprintf(ts->outfile, "%s(", name);
    for (i = 0; i < nargs; i++) {
        if (s_args[i] != NULL)
            len += fprintf(ts->outfile, "%s", s_args[i]);
        else
            len += fprintf(ts->outfile,
                           "<missing argument>");
        len += fprintf(ts->outfile, "%s", i < (nargs - 1) ?
                       "," : "");
    }
    len += fprintf(ts->outfile, ")");
    for (i = 0; i < 6 - (len / 8); i++)
        fprintf(ts->outfile, "\t");

    if (error == TARGET_ERESTART)
        fprintf(ts->outfile, " ERESTART\n");
    else if (error == TARGET_EJUSTRETURN)
        fprintf(ts->outfile, " EJUSTRETURN\n");
    else if (error != 0) {
        fprintf(ts->outfile, " ERR#%d '%s'\n", error, strerror(error));
    }
#if TARGET_ABI_BITS == 32
    else if (ts->cs.sc->ret_type == 2) {
        off_t off;
#ifdef TARGET_BIG_ENDIAN
        off = (off_t)ret << 32 | ret;
#else
        off = (off_t)ret2 << 32 | ret;
#endif
        fprintf(ts->outfile, " = %jd (0x%jx)", (intmax_t)off, (intmax_t)off);
    }
#endif
    else {
        fprintf(ts->outfile, " = %jd (0x%jx)", (intmax_t)ret, (intmax_t)ret);
    }
}

void record_syscall_ret(TaskState *ts, int num, abi_ulong ret, abi_ulong ret2)
{
    int error = 0;

    for (u_int i = 0; i < ts->cs.nargs; i++) {
        if (ts->cs.sc->args[i].type & OUT) {
            ts->cs.s_args[i] = print_arg(ts, &ts->cs.sc->args[i], ts->cs.args, ret, ret2);
        }
    }

    /* Convert the Linux convention for errno to BSD */
    if ((abi_long)ret < 0 && (abi_long)ret >= -511) {
        error = -(abi_long)ret;
        ret = ret2 = (abi_ulong)-1;
    }
    print_syscall_ret(ts, ret, ret2, error);
    gemu_log("%s\n", ts->trace_buf);
    free_syscall(ts);
}

/* XXX */

static void
print_signal(abi_ulong arg, int last)
{
    const char *signal_name = NULL;
    switch (arg) {
    case TARGET_SIGHUP:
        signal_name = "SIGHUP";
        break;
    case TARGET_SIGINT:
        signal_name = "SIGINT";
        break;
    case TARGET_SIGQUIT:
        signal_name = "SIGQUIT";
        break;
    case TARGET_SIGILL:
        signal_name = "SIGILL";
        break;
    case TARGET_SIGABRT:
        signal_name = "SIGABRT";
        break;
    case TARGET_SIGFPE:
        signal_name = "SIGFPE";
        break;
    case TARGET_SIGKILL:
        signal_name = "SIGKILL";
        break;
    case TARGET_SIGSEGV:
        signal_name = "SIGSEGV";
        break;
    case TARGET_SIGPIPE:
        signal_name = "SIGPIPE";
        break;
    case TARGET_SIGALRM:
        signal_name = "SIGALRM";
        break;
    case TARGET_SIGTERM:
        signal_name = "SIGTERM";
        break;
    case TARGET_SIGUSR1:
        signal_name = "SIGUSR1";
        break;
    case TARGET_SIGUSR2:
        signal_name = "SIGUSR2";
        break;
    case TARGET_SIGCHLD:
        signal_name = "SIGCHLD";
        break;
    case TARGET_SIGCONT:
        signal_name = "SIGCONT";
        break;
    case TARGET_SIGSTOP:
        signal_name = "SIGSTOP";
        break;
    case TARGET_SIGTTIN:
        signal_name = "SIGTTIN";
        break;
    case TARGET_SIGTTOU:
        signal_name = "SIGTTOU";
        break;
    }
    if (signal_name == NULL) {
        gemu_log("signal %ld", (long)arg);
        return;
    }
    gemu_log("%s", signal_name);
}

void print_taken_signal(int target_signum, const target_siginfo_t *tinfo)
{
    /*
     * Print the strace output for a signal being taken:
     * --- SIGSEGV {si_signo=SIGSEGV, si_code=SI_KERNEL, si_addr=0} ---
     */
    gemu_log("%d ", getpid());
    gemu_log("--- ");
    print_signal(target_signum, 1);
    gemu_log(" ---\n");
}
