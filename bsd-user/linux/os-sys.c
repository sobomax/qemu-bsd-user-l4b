/*
 *  FreeBSD sysctl() and sysarch() system call emulation
 *
 *  Copyright (c) 2013-15 Stacey D. Son
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

#include "qemu/osdep.h"
#include "qemu.h"
#include "qemu-bsd.h"
#include "signal-common.h"
#include <sys/param.h>
#include <sys/mount.h>
#if !defined(__linux__)
#include <sys/sysctl.h>
#else
#include <sys/utsname.h>
#endif
#include <sys/user.h>   /* For struct kinfo_* */
#include <net/route.h>  /* For rt_msghdr */
#include <net/if.h>     /* For *_msghdr */
#if !defined(__linux__)
#include <net/if_dl.h>  /* For sockaddr_dl */
#elif defined(HAVE_BSD_STRING_H)
#include <bsd/string.h>
#endif

#include <string.h>

#include "bsd-proc.h"

#include "target_arch_sysarch.h"
#include "target_os_vmparam.h"
#include "target_os_user.h"

#include "os-dev_proc.h"

/*
 * Length for the fixed length types.
 * 0 means variable length for strings and structures
 * Compare with sys/kern_sysctl.c ctl_size
 * Note: Not all types appear to be used in-tree.
 */
static const int guest_ctl_size[CTLTYPE + 1] = {
	[CTLTYPE_INT] = sizeof(abi_int),
	[CTLTYPE_UINT] = sizeof(abi_uint),
	[CTLTYPE_LONG] = sizeof(abi_long),
	[CTLTYPE_ULONG] = sizeof(abi_ulong),
	[CTLTYPE_S8] = sizeof(int8_t),
	[CTLTYPE_S16] = sizeof(int16_t),
	[CTLTYPE_S32] = sizeof(int32_t),
	[CTLTYPE_S64] = sizeof(int64_t),
	[CTLTYPE_U8] = sizeof(uint8_t),
	[CTLTYPE_U16] = sizeof(uint16_t),
	[CTLTYPE_U32] = sizeof(uint32_t),
	[CTLTYPE_U64] = sizeof(uint64_t),
};

static const int host_ctl_size[CTLTYPE + 1] = {
	[CTLTYPE_INT] = sizeof(int),
	[CTLTYPE_UINT] = sizeof(u_int),
	[CTLTYPE_LONG] = sizeof(long),
	[CTLTYPE_ULONG] = sizeof(u_long),
	[CTLTYPE_S8] = sizeof(int8_t),
	[CTLTYPE_S16] = sizeof(int16_t),
	[CTLTYPE_S32] = sizeof(int32_t),
	[CTLTYPE_S64] = sizeof(int64_t),
	[CTLTYPE_U8] = sizeof(uint8_t),
	[CTLTYPE_U16] = sizeof(uint16_t),
	[CTLTYPE_U32] = sizeof(uint32_t),
	[CTLTYPE_U64] = sizeof(uint64_t),
};

#define _STR(x) #x
#define _2STR(x) _STR(x)
#define FreeBSD_version_str _2STR(__FreeBSD_version)

static char osrelease_str[16] = FreeBSD_version_str;
static char kversion_str[64] = (FREEBSD_ABI_VENDOR " " FreeBSD_version_str);
static abi_ulong pagesizes[2] = {4096, 1024 * 1024 * 2};

#define QEMU_REL_SUF "-QEMU-L4B"
#define QEMU_REL_POS 4

static_assert(sizeof(QEMU_REL_SUF) < sizeof(osrelease_str) - QEMU_REL_POS,
             "osrelease_str is too small");

static void
patch_osrelease(char *osp, off_t at, size_t olen) {
    assert (olen >= (QEMU_REL_POS + sizeof(QEMU_REL_SUF) + at));
    osp += at;
    assert(osp[2] == '0' && osp[QEMU_REL_POS] == '0');
    osp[2] = '.';
    memcpy(osp + QEMU_REL_POS, QEMU_REL_SUF, sizeof(QEMU_REL_SUF));
}

void
init_bsd_sysctl(void)
{
    patch_osrelease(osrelease_str, 0, sizeof(osrelease_str));
    patch_osrelease(kversion_str, sizeof(FREEBSD_ABI_VENDOR), sizeof(kversion_str));
    for (abi_ulong *ps = pagesizes; (void *)ps < ((void *)pagesizes+sizeof(pagesizes)); ps++) {
        *ps = tswapal(*ps);
    }
}

#ifdef TARGET_ABI32
/*
 * Limit the amount of available memory to be most of the 32-bit address
 * space. 0x100c000 was arrived at through trial and error as a good
 * definition of 'most'.
 */
static const abi_ulong guest_max_mem = UINT32_MAX - 0x100c000 + 1;

static abi_ulong cap_memory(uint64_t mem)
{
    return MIN(guest_max_mem, mem);
}
#endif

static abi_ulong scale_to_guest_pages(uint64_t pages)
{
    /* Scale pages from host to guest */
    pages = muldiv64(pages, qemu_real_host_page_size(), TARGET_PAGE_SIZE);
#ifdef TARGET_ABI32
    /* cap pages if need be */
    pages = MIN(pages, guest_max_mem / (abi_ulong)TARGET_PAGE_SIZE);
#endif
    return pages;
}

#ifdef TARGET_ABI32
/* Used only for TARGET_ABI32 */
static abi_long h2g_long_sat(long l)
{
    if (l > INT32_MAX) {
        l = INT32_MAX;
    } else if (l < INT32_MIN) {
        l = INT32_MIN;
    }
    return l;
}

static abi_ulong h2g_ulong_sat(u_long ul)
{
    return MIN(ul, UINT32_MAX);
}
#endif

#ifdef TARGET_ARM
/* For reading target arch */
#include <libelf.h>
#include <gelf.h>

static int
get_target_arch(TaskState *ts, char **machine_arch)
{
    int fd, rv;
    const char *buf, *end;
    Elf *ep;
    Elf_Data *dp;
    Elf_Note *note;
    Elf_Scn *scnp;
    GElf_Shdr shdr;

    ep = NULL;
    rv = 1;
    fd = open(ts->bprm->fullpath, O_RDONLY);
    if (fd < 0)
        return (1);

    if (elf_version(EV_CURRENT) == EV_NONE)
        goto out;

    ep = elf_begin(fd, ELF_C_READ, NULL);
    if (ep == NULL)
        goto out;

    scnp = elf_getscn(ep, 0);
    if (scnp == NULL)
        goto out;
    do {
        if (gelf_getshdr(scnp, &shdr) != &shdr)
            continue;
        if (shdr.sh_type != SHT_NOTE)
            continue;
        if ((dp = elf_getdata(scnp, NULL)) == NULL)
            continue;

        buf = dp->d_buf;
        end = buf + dp->d_size;
        while (buf < end) {
            if (buf + sizeof(*note) > end)
                break;
            note = (Elf_Note *)(uintptr_t) buf;
            if (note->n_type == NT_FREEBSD_ARCH_TAG) {
                buf += sizeof(*note) + roundup2(note->n_namesz, 4);
                *machine_arch = strdup(buf);
                rv = 0;
                goto out;
            }
            buf += sizeof(*note) + roundup2(note->n_namesz, 4) +
              roundup2(note->n_descsz, 4);
        }
    } while ((scnp = elf_nextscn(ep, scnp)) != NULL);
out:
    if (ep != NULL)
        elf_end(ep);
    close(fd);
    return (rv);
}
#endif

#if !defined(__linux__)
static void
host_to_target_kinfo_proc(struct target_kinfo_proc *tki, struct kinfo_proc *hki)
{
    int i;

    __put_user(sizeof(struct target_kinfo_proc), &tki->ki_structsize);
    __put_user(hki->ki_layout, &tki->ki_layout);

    /* Some of these are used as flags (e.g. ki_fd == NULL in procstat). */
    tki->ki_args = tswapal((abi_ulong)(uintptr_t)hki->ki_args);
    tki->ki_paddr = tswapal((abi_ulong)(uintptr_t)hki->ki_paddr);
    tki->ki_addr = tswapal((abi_ulong)(uintptr_t)hki->ki_addr);
    tki->ki_tracep = tswapal((abi_ulong)(uintptr_t)hki->ki_tracep);
    tki->ki_textvp = tswapal((abi_ulong)(uintptr_t)hki->ki_textvp);
    tki->ki_fd = tswapal((abi_ulong)(uintptr_t)hki->ki_fd);
    tki->ki_vmspace = tswapal((abi_ulong)(uintptr_t)hki->ki_vmspace);
    tki->ki_wchan = tswapal((abi_ulong)(uintptr_t)hki->ki_wchan);

    __put_user(hki->ki_pid, &tki->ki_pid);
    __put_user(hki->ki_ppid, &tki->ki_ppid);
    __put_user(hki->ki_pgid, &tki->ki_pgid);
    __put_user(hki->ki_tpgid, &tki->ki_tpgid);
    __put_user(hki->ki_sid, &tki->ki_sid);
    __put_user(hki->ki_tsid, &tki->ki_tsid);
    __put_user(hki->ki_jobc, &tki->ki_jobc);
    __put_user(hki->ki_tdev, &tki->ki_tdev);

    host_to_target_sigset(&tki->ki_siglist, &hki->ki_siglist);
    host_to_target_sigset(&tki->ki_sigmask, &hki->ki_sigmask);
    host_to_target_sigset(&tki->ki_sigignore, &hki->ki_sigignore);
    host_to_target_sigset(&tki->ki_sigcatch, &hki->ki_sigcatch);

    __put_user(hki->ki_uid, &tki->ki_uid);
    __put_user(hki->ki_ruid, &tki->ki_ruid);
    __put_user(hki->ki_svuid, &tki->ki_svuid);
    __put_user(hki->ki_rgid, &tki->ki_rgid);
    __put_user(hki->ki_svgid, &tki->ki_svgid);
    __put_user(hki->ki_ngroups, &tki->ki_ngroups);

    for (i=0; i < TARGET_KI_NGROUPS; i++)
        __put_user(hki->ki_groups[i], &tki->ki_groups[i]);

    __put_user(hki->ki_size, &tki->ki_size);

    __put_user(hki->ki_rssize, &tki->ki_rssize);
    __put_user(hki->ki_swrss, &tki->ki_swrss);
    __put_user(hki->ki_tsize, &tki->ki_tsize);
    __put_user(hki->ki_dsize, &tki->ki_dsize);
    __put_user(hki->ki_ssize, &tki->ki_ssize);

    __put_user(hki->ki_xstat, &tki->ki_xstat);
    __put_user(hki->ki_acflag, &tki->ki_acflag);

    __put_user(hki->ki_pctcpu, &tki->ki_pctcpu);

    __put_user(hki->ki_estcpu, &tki->ki_estcpu);
    __put_user(hki->ki_slptime, &tki->ki_slptime);
    __put_user(hki->ki_swtime, &tki->ki_swtime);
    __put_user(hki->ki_cow, &tki->ki_cow);
    __put_user(hki->ki_runtime, &tki->ki_runtime);

    __put_user(hki->ki_start.tv_sec, &tki->ki_start.tv_sec);
    __put_user(hki->ki_start.tv_usec, &tki->ki_start.tv_usec);
    __put_user(hki->ki_childtime.tv_sec, &tki->ki_childtime.tv_sec);
    __put_user(hki->ki_childtime.tv_usec, &tki->ki_childtime.tv_usec);

    __put_user(hki->ki_flag, &tki->ki_flag);
    __put_user(hki->ki_kiflag, &tki->ki_kiflag);

    __put_user(hki->ki_traceflag, &tki->ki_traceflag);
    __put_user(hki->ki_stat, &tki->ki_stat);
    __put_user(hki->ki_nice, &tki->ki_nice);
    __put_user(hki->ki_lock, &tki->ki_lock);
    __put_user(hki->ki_rqindex, &tki->ki_rqindex);
    __put_user(hki->ki_oncpu_old, &tki->ki_oncpu_old);
    __put_user(hki->ki_lastcpu_old, &tki->ki_lastcpu_old);

    strncpy(tki->ki_tdname, hki->ki_tdname, TARGET_TDNAMLEN+1);
    strncpy(tki->ki_wmesg, hki->ki_wmesg, TARGET_WMESGLEN+1);
    strncpy(tki->ki_login, hki->ki_login, TARGET_LOGNAMELEN+1);
    strncpy(tki->ki_lockname, hki->ki_lockname, TARGET_LOCKNAMELEN+1);
    strncpy(tki->ki_comm, hki->ki_comm, TARGET_COMMLEN+1);
    strncpy(tki->ki_emul, hki->ki_emul, TARGET_KI_EMULNAMELEN+1);
    strncpy(tki->ki_loginclass, hki->ki_loginclass, TARGET_LOGINCLASSLEN+1);

    __put_user(hki->ki_oncpu, &tki->ki_oncpu);
    __put_user(hki->ki_lastcpu, &tki->ki_lastcpu);
    __put_user(hki->ki_tracer, &tki->ki_tracer);
    __put_user(hki->ki_flag2, &tki->ki_flag2);
    __put_user(hki->ki_fibnum, &tki->ki_fibnum);
    __put_user(hki->ki_cr_flags, &tki->ki_cr_flags);
    __put_user(hki->ki_jid, &tki->ki_jid);
    __put_user(hki->ki_numthreads, &tki->ki_numthreads);
    __put_user(hki->ki_tid, &tki->ki_tid);

    memcpy(&tki->ki_pri, &hki->ki_pri, sizeof(struct target_priority));

    h2g_rusage(&hki->ki_rusage, &tki->ki_rusage);
    h2g_rusage(&hki->ki_rusage_ch, &tki->ki_rusage_ch);

    __put_user(((uintptr_t)hki->ki_pcb), &tki->ki_pcb);
    __put_user(((uintptr_t)hki->ki_kstack), &tki->ki_kstack);
    __put_user(((uintptr_t)hki->ki_udata), &tki->ki_udata);
    __put_user(((uintptr_t)hki->ki_tdaddr), &tki->ki_tdaddr);

    __put_user(hki->ki_sflag, &tki->ki_sflag);
    __put_user(hki->ki_tdflags, &tki->ki_tdflags);
}
#endif

abi_long
do_sysctl_kern_getprocs(int op, int arg, size_t olen,
        struct target_kinfo_proc *tki, size_t *tlen)
{
#if !defined(__linux__)
    abi_long ret;
    struct kinfo_proc *kipp;
    int mib[4], num, i, miblen;
    size_t len;

    if (tlen == NULL)
        return -TARGET_EINVAL;

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = op;
    mib[3] = arg;

    miblen = (op == KERN_PROC_ALL || op == KERN_PROC_PROC) ?  3 : 4;

    len = 0;
    ret = get_errno(sysctl(mib, miblen, NULL, &len, NULL, 0));
    if (is_error(ret))
        return ret;

    num = len / sizeof(*kipp);
    *tlen = num * sizeof(struct target_kinfo_proc);
    if (tki == NULL)
        return ret;

    if (olen < *tlen)
        return -TARGET_EINVAL;

    kipp = g_malloc(len);
    if (kipp == NULL)
        return -TARGET_ENOMEM;
    ret = get_errno(sysctl(mib, miblen, kipp, &len, NULL, 0));
    num = len / sizeof(*kipp);
    *tlen = num * sizeof(struct target_kinfo_proc);
    if (len % sizeof(*kipp) != 0 || kipp->ki_structsize != sizeof(*kipp)) {
        ret = -TARGET_EINVAL; /* XXX */
    } else if (!is_error(ret)) {
        for(i=0; i < num; i++)
            host_to_target_kinfo_proc(&tki[i], &kipp[i]);
    }

    g_free(kipp);
    return ret;
#else
    abort();
#endif
}

#if !defined(__linux__)
static void
host_to_target_kinfo_file(struct target_kinfo_file *tkif,
        struct kinfo_file *hkif)
{
    int type = hkif->kf_type;

    __put_user(hkif->kf_structsize, &tkif->kf_structsize);
    __put_user(hkif->kf_type, &tkif->kf_type);
    __put_user(hkif->kf_fd, &tkif->kf_fd);
    __put_user(hkif->kf_ref_count, &tkif->kf_ref_count);
    __put_user(hkif->kf_flags, &tkif->kf_flags);
    __put_user(hkif->kf_offset, &tkif->kf_offset);
    switch (type) {
    case TARGET_KF_TYPE_FIFO:
    case TARGET_KF_TYPE_SHM:
    case TARGET_KF_TYPE_VNODE:
        __put_user(hkif->kf_un.kf_file.kf_file_type,
                &tkif->kf_un.kf_file.kf_file_type);
        __put_user(hkif->kf_un.kf_file.kf_file_fsid,
                &tkif->kf_un.kf_file.kf_file_fsid);
        __put_user(hkif->kf_un.kf_file.kf_file_rdev,
                &tkif->kf_un.kf_file.kf_file_rdev);
        __put_user(hkif->kf_un.kf_file.kf_file_fileid,
                &tkif->kf_un.kf_file.kf_file_fileid);
        __put_user(hkif->kf_un.kf_file.kf_file_size,
                &tkif->kf_un.kf_file.kf_file_size);
        __put_user(hkif->kf_un.kf_file.kf_file_fsid_freebsd11,
                &tkif->kf_un.kf_file.kf_file_fsid_freebsd11);
        __put_user(hkif->kf_un.kf_file.kf_file_rdev_freebsd11,
                &tkif->kf_un.kf_file.kf_file_rdev_freebsd11);
        __put_user(hkif->kf_un.kf_file.kf_file_mode,
                &tkif->kf_un.kf_file.kf_file_mode);
        break;

    case TARGET_KF_TYPE_SOCKET:
        __put_user(hkif->kf_un.kf_sock.kf_sock_domain0,
                &tkif->kf_un.kf_sock.kf_sock_domain0);
        __put_user(hkif->kf_un.kf_sock.kf_sock_type0,
                &tkif->kf_un.kf_sock.kf_sock_type0);
        __put_user(hkif->kf_un.kf_sock.kf_sock_protocol0,
                &tkif->kf_un.kf_sock.kf_sock_protocol0);
/*  XXX - Implement copy function for sockaddr_storage
        host_to_target_copy_sockaddr_storage(
                &hkif->kf_un.kf_file.kf_sa_local,
                &kif->kf_un.kf_file.kf_sa_local);
        host_to_target_copy_sockaddr_storage(
                &hkif->kf_un.kf_file.kf_sa_peer,
                &kif->kf_un.kf_file.kf_sa_peer);
*/
        __put_user(hkif->kf_un.kf_sock.kf_sock_pcb,
                &tkif->kf_un.kf_sock.kf_sock_pcb);
        __put_user(hkif->kf_un.kf_sock.kf_sock_inpcb,
                &tkif->kf_un.kf_sock.kf_sock_inpcb);
        __put_user(hkif->kf_un.kf_sock.kf_sock_unpconn,
                &tkif->kf_un.kf_sock.kf_sock_unpconn);
        __put_user(hkif->kf_un.kf_sock.kf_sock_snd_sb_state,
                &tkif->kf_un.kf_sock.kf_sock_snd_sb_state);
        __put_user(hkif->kf_un.kf_sock.kf_sock_rcv_sb_state,
                &tkif->kf_un.kf_sock.kf_sock_rcv_sb_state);
        break;

    case TARGET_KF_TYPE_PIPE:
        __put_user(hkif->kf_un.kf_pipe.kf_pipe_addr,
                &tkif->kf_un.kf_pipe.kf_pipe_addr);
        __put_user(hkif->kf_un.kf_pipe.kf_pipe_peer,
                &tkif->kf_un.kf_pipe.kf_pipe_peer);
        __put_user(hkif->kf_un.kf_pipe.kf_pipe_buffer_cnt,
                &tkif->kf_un.kf_pipe.kf_pipe_buffer_cnt);
        break;

    case TARGET_KF_TYPE_SEM:
        __put_user(hkif->kf_un.kf_sem.kf_sem_value,
                &tkif->kf_un.kf_sem.kf_sem_value);
        __put_user(hkif->kf_un.kf_sem.kf_sem_mode,
                &tkif->kf_un.kf_sem.kf_sem_mode);
        break;

    case TARGET_KF_TYPE_PTS:
        __put_user(hkif->kf_un.kf_pts.kf_pts_dev_freebsd11,
                &tkif->kf_un.kf_pts.kf_pts_dev_freebsd11);
        __put_user(hkif->kf_un.kf_pts.kf_pts_dev,
                &tkif->kf_un.kf_pts.kf_pts_dev);
        break;

    case TARGET_KF_TYPE_PROCDESC:
        __put_user(hkif->kf_un.kf_proc.kf_pid,
                &tkif->kf_un.kf_proc.kf_pid);
        break;


    case TARGET_KF_TYPE_CRYPTO:
    case TARGET_KF_TYPE_KQUEUE:
    case TARGET_KF_TYPE_MQUEUE:
    case TARGET_KF_TYPE_NONE:
    case TARGET_KF_TYPE_UNKNOWN:
    default:
        /* Do nothing. */
        break;
    }
    __put_user(hkif->kf_status, &tkif->kf_status);
    for (int i = 0; i < (CAP_RIGHTS_VERSION + 2); i++)
        __put_user(hkif->kf_cap_rights.cr_rights[i],
                &tkif->kf_cap_rights.cr_rights[i]);
    strncpy(tkif->kf_path, hkif->kf_path, sizeof(tkif->kf_path));
}
#endif

abi_long
do_sysctl_kern_proc_filedesc(int pid, size_t olen,
        struct target_kinfo_file *tkif, size_t *tlen)
{
#if !defined(__linux__)
    abi_long ret;
    int mib[4], sz;
    size_t len;
    char *buf, *bp, *eb, *tp;
    struct kinfo_file *kf, kif;
    struct target_kinfo_file target_kif;

    if (tlen == NULL) {
        return -TARGET_EINVAL;
    }

    len = 0;
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_FILEDESC;
    mib[3] = pid;

    ret = get_errno(sysctl(mib, 4, NULL, &len, NULL, 0));
    if (is_error(ret)) {
        return ret;
    }
    if (tkif == NULL) {
        *tlen = len;
        return ret;
    }
    len = len * 4 / 3;
    buf = g_malloc(len);
    if (buf == NULL) {
        return -TARGET_ENOMEM;
    }

    /*
     * Count the number of records.
     *
     * Given that the kinfo_file information returned by
     * the kernel may be different sizes per record we have
     * to read it in and count the variable length records
     * by walking them.
     */
    ret = get_errno(sysctl(mib, 4, buf, &len, NULL, 0));
    if (is_error(ret)) {
        g_free(buf);
        return ret;
    }
    *tlen = len;
    bp = buf;
    eb = buf + len;
    while (bp < eb) {
        kf = (struct kinfo_file *)(uintptr_t)bp;
        bp += kf->kf_structsize;
    }
    if (olen < *tlen) {
        g_free(buf);
        return -TARGET_EINVAL;
    }

    /*
     * Unpack the records from the kernel into full length records
     * and byte swap, if needed.
     */
    bp = buf;
    eb = buf + len;
    tp = (char *)tkif;
    while (bp < eb) {
        kf = (struct kinfo_file *)(uintptr_t)bp;
        sz = kf->kf_structsize;
        /* Copy/expand into a zeroed buffer */
        memset(&kif, 0, sizeof(kif));
        memcpy(&kif, kf, sz);
        /* Byte swap and copy into a target buffer. */
        host_to_target_kinfo_file(&target_kif, &kif);
        /* Copy target buffer to user buffer and pack */
        memcpy(tp, &target_kif, sz);
        /* Advance to next packed record. */
        bp += sz;
        /* Advance to next packed, target record. */
        tp += sz;
    }

    g_free(buf);
    return ret;
#else
    abort();
#endif
}

#if !defined(__linux__)
static void
host_to_target_kinfo_vmentry(struct target_kinfo_vmentry *tkve,
        struct kinfo_vmentry *hkve)
{

    __put_user(hkve->kve_structsize, &tkve->kve_structsize);
    __put_user(hkve->kve_type, &tkve->kve_type);
    __put_user(hkve->kve_start, &tkve->kve_start);
    __put_user(hkve->kve_end, &tkve->kve_end);
    __put_user(hkve->kve_offset, &tkve->kve_offset);
    __put_user(hkve->kve_vn_fileid, &tkve->kve_vn_fileid);
    __put_user(hkve->kve_vn_fsid_freebsd11, &tkve->kve_vn_fsid_freebsd11);
    __put_user(hkve->kve_vn_fsid, &tkve->kve_vn_fsid);
    __put_user(hkve->kve_flags, &tkve->kve_flags);
    __put_user(hkve->kve_resident, &tkve->kve_resident);
    __put_user(hkve->kve_private_resident, &tkve->kve_private_resident);
    __put_user(hkve->kve_protection, &tkve->kve_protection);
    __put_user(hkve->kve_ref_count, &tkve->kve_ref_count);
    __put_user(hkve->kve_shadow_count, &tkve->kve_shadow_count);
    __put_user(hkve->kve_vn_type, &tkve->kve_vn_type);
    __put_user(hkve->kve_vn_size, &tkve->kve_vn_size);
    __put_user(hkve->kve_vn_rdev_freebsd11, &tkve->kve_vn_rdev_freebsd11);
    __put_user(hkve->kve_vn_rdev, &tkve->kve_vn_rdev);
    __put_user(hkve->kve_vn_mode, &tkve->kve_vn_mode);
    __put_user(hkve->kve_status, &tkve->kve_status);
    strncpy(tkve->kve_path, hkve->kve_path, sizeof(tkve->kve_path));
}
#endif

abi_long
do_sysctl_kern_proc_vmmap(int pid, size_t olen,
        struct target_kinfo_vmentry *tkve, size_t *tlen)
{
#if !defined(__linux__)
    abi_long ret;
    int mib[4], sz;
    size_t len;
    char *buf, *bp, *eb, *tp;
    struct kinfo_vmentry *kve, kvme;
    struct target_kinfo_vmentry target_kvme;

    if (tlen == NULL) {
        return -TARGET_EINVAL;
    }

    len = 0;
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_VMMAP;
    mib[3] = pid;

    ret = get_errno(sysctl(mib, 4, NULL, &len, NULL, 0));
    if (is_error(ret)) {
        return ret;
    }
    if (tkve == NULL) {
        *tlen = len;
        return ret;
    }
    len = len * 4 / 3;
    buf = g_malloc(len);
    if (buf == NULL) {
        return -TARGET_ENOMEM;
    }

    /*
     * Count the number of records.
     *
     * Given that the kinfo_file information returned by
     * the kernel may be differents sizes per record we have
     * to read it in and count the variable length records
     * by walking them.
     */
    ret = get_errno(sysctl(mib, 4, buf, &len, NULL, 0));
    if (is_error(ret)) {
        g_free(buf);
        return ret;
    }
    *tlen = len;
    bp = buf;
    eb = buf + len;
    while (bp < eb) {
        kve = (struct kinfo_vmentry *)(uintptr_t)bp;
        bp += kve->kve_structsize;
    }
    if (olen < *tlen) {
        g_free(buf);
        return -TARGET_EINVAL;
    }

    /*
     * Unpack the records from the kernel into full length records
     * and byte swap, if needed.
     */
    bp = buf;
    eb = buf + len;
    tp = (char *)tkve;
    while (bp < eb) {
        kve = (struct kinfo_vmentry *)(uintptr_t)bp;
        sz = kve->kve_structsize;
        /* Copy/expand into a zeroed buffer */
        memset(&kvme, 0, sizeof(kvme));
        memcpy(&kvme, kve, sz);
        /* Byte swap and copy into a target aligned buffer. */
        host_to_target_kinfo_vmentry(&target_kvme, &kvme);
        /* Copy target buffer to user buffer, packed. */
        memcpy(tp, &target_kvme, sz);
        /* Advance to next packed record. */
        bp += sz;
        /* Advance to next packed, target record. */
        tp += sz;
    }

    g_free(buf);
    return ret;
#else
    abort();
#endif
}

#if !defined(__linux__)
/* #define ROUTETABLE_DEBUG */
#ifdef ROUTETABLE_DEBUG

#define rtprintf(...) printf(__VA_ARGS__)

static void
hexdump(const char *prefix, const void *sbuf, int len)
{
    const char *buf = sbuf;
    int i;
    for (i = 0; i < len; i++) {
        if (i && (i & 7) == 0) {
            printf(" ");
        }
        if (i && (i & 15) == 0) {
            printf("\n");
        }
        if ((i & 15) == 0) {
            printf("%s: ", prefix);
        }
        rtprintf("%02X ", (unsigned char)buf[i]);
    }
    rtprintf("\n");
}

#else
static void
hexdump(const char *prefix, const void *sbuf, int len)
{
}
#define rtprintf(...)
#endif
#endif

/* Assign byteswapped value to target, autodetecting size. */
#define PUT(t, v)                  \
    switch (sizeof(t)) {           \
    case 1:                        \
        t = v;                     \
        break;                     \
    case 2:                        \
        t = tswap16(v);            \
        break;                     \
    case 4:                        \
        t = tswap32(v);            \
        break;                     \
    case 8:                        \
        t = tswap64(v);            \
        break;                     \
    default:                       \
        abort();                   \
    }

/* SA_SIZE macro, adjusted for target word size. */
#define TARGET_SA_SIZE(sa)                               \
 ((!(sa) || ((struct sockaddr *)(sa))->sa_len == 0) ?    \
   sizeof(abi_long)            :                         \
   1 + ((((struct sockaddr *)(sa))->sa_len - 1) | (sizeof(abi_long) - 1)) \
 )

#if !defined(__linux__)
/*
 * Given a bitmap of sockaddr types, and a pointer to the host buffer, realign
 * the sockaddr's into the target buffer, and byteswap field members as needed.
 * Return value is the length of the repacked buffer.
 */
static int
host_to_target_copy_sockaddrs(int addrs, char *hbuf, char *tbuf)
{
    int tlen = 0;
    int i;
    struct sockaddr *sa;

    rtprintf("copy_sockaddrs: host=%p, target=%p\n", hbuf, tbuf);

    for (i = 0; i < RTAX_MAX; i++) {
        /* If the bitmap for this sockaddr type is clear, skip it. */
        if ((addrs & (1 << i)) == 0) {
            continue;
        }

        sa = (struct sockaddr *)hbuf;

        rtprintf(
            "sockaddr, addr=%02X, family=%d, hlen=%d, hsize=%ld, tsize=%ld\n",
            (1 << i), sa->sa_family, sa->sa_len,
            SA_SIZE(sa), TARGET_SA_SIZE(sa));

        memcpy(tbuf, hbuf, SA_SIZE(sa));

        /* Byteswap fields for address families that require it. */
        switch (sa->sa_family) {
        case AF_LINK: {
            struct sockaddr_dl *dl;
            dl = (struct sockaddr_dl *)tbuf;
            PUT(dl->sdl_index, dl->sdl_index);
            break;
        }

        case AF_INET6: {
            struct sockaddr_in6 *sin6;
            sin6 = (struct sockaddr_in6 *)tbuf;
            PUT(sin6->sin6_port, sin6->sin6_port);
            PUT(sin6->sin6_flowinfo, sin6->sin6_flowinfo);
            PUT(sin6->sin6_scope_id, sin6->sin6_scope_id);
            break;
        }

        }

        hexdump("sh", hbuf, SA_SIZE(sa));
        hexdump("st", tbuf, TARGET_SA_SIZE(sa));

        /* Increment pointers and counters by the appropriate amounts. */
        hbuf += SA_SIZE(sa);
        tbuf += TARGET_SA_SIZE(sa);
        tlen += TARGET_SA_SIZE(sa);
    }
    return tlen;
}
#endif

static abi_long
do_sysctl_net_routetable_iflistl(int32_t *snamep, size_t namelen, size_t olen,
                                 char *tbuf, size_t *tlen)
{
#if !defined(__linux__)
    abi_long ret;

    char *bp, *ep, *tp;
    struct rt_msghdr *rtm, *trtm;
    char *buf = NULL;
    size_t len;

    /* NET_RT_IFLISTL */

    /*
     * This sysctl returns a blob composed of a list of *_msghdr structs (of a
     * few different types and sizes), each followed by a list of sockaddr_*
     * structs (of a few different types and sizes).  Many members in each
     * struct will need to be byte-swapped, and unfortunately, the sockaddr
     * structs are aligned in a MD way, so the byteswapping can't be done
     * in-place.  We have to rebuild a new correctly-aligned blob to pass back
     * to the target.
     */

    rtprintf("in sysctl1, olen=%zd, tlen=%zd, tbuf=%p\n",
             olen, *tlen, tbuf);

    /*
     * extend target's requested size by 25% to be on the safe side, even though
     * repacking for a 32-bit target shrinks the blob.
     */
    len = *tlen * 4 / 3;
    if (len && tbuf) {
        buf = g_malloc(len);
        if (buf == NULL) {
            return -TARGET_ENOMEM;
        }
        memset(buf, 0, len);
    }

    rtprintf("in sysctl2, len=%zd, buf=%p\n",
             len, buf);
    ret = get_errno(sysctl(snamep, namelen, buf, &len, NULL, 0));
    rtprintf("in sysctl3, ret=%ld, len=%zd\n",
             ret, len);

    /*
     * If the target was just probing for a good size to use, extend that by 25%
     * too.
     */
    if (olen == 0 || tbuf == NULL) {
        *tlen = len * 4 / 3;
        return ret;
    }

    if (is_error(ret)) {
        g_free(buf);
        return ret;
    }

    rtprintf("in sysctl4, len=%zd, tlen=%zd, buf=%p, tbuf = %p\n",
             len, *tlen, buf, tbuf);

    /*
     * Loop through the blob, picking off routing messages and copying them to
     * tbuf, swapping struct members as needed.
     */

    bp = buf;
    ep = buf + len;
    tp = tbuf;
    while (bp < ep) {
        int sa_len;
        int tmsglen;
        rtm = (struct rt_msghdr *)bp;
        trtm = (struct rt_msghdr *)tp;

        rtprintf("before, host=%p, target=%p, msglen=%d\n",
                 bp, tp, rtm->rtm_msglen);
        hexdump("h", rtm, rtm->rtm_msglen);

        switch (rtm->rtm_type) {
        case RTM_IFINFO: {
            struct if_msghdrl *ifm, *tifm;
            ifm = (struct if_msghdrl *)bp;
            tifm = (struct if_msghdrl *)tp;

            rtprintf("interface. index=%d, msglen=%d, len=%d, data_off=%d\n",
                     ifm->ifm_index, ifm->ifm_msglen, ifm->ifm_len,
                     ifm->ifm_data_off);

            memcpy(tifm, ifm, sizeof(*ifm));
            /* copy and repack sockaddrs */
            sa_len = host_to_target_copy_sockaddrs(ifm->ifm_addrs,
                                                   IF_MSGHDRL_RTA(ifm),
                                                   IF_MSGHDRL_RTA(tifm));

            /* byteswap */
            PUT(tifm->ifm_addrs, ifm->ifm_addrs);
            PUT(tifm->ifm_flags, ifm->ifm_flags);
            PUT(tifm->ifm_index, ifm->ifm_index);
            PUT(tifm->ifm_len, ifm->ifm_len);
            PUT(tifm->ifm_data_off, ifm->ifm_data_off);
            PUT(tifm->ifm_data.ifi_datalen, ifm->ifm_data.ifi_datalen);
            PUT(tifm->ifm_data.ifi_mtu, ifm->ifm_data.ifi_mtu);
            PUT(tifm->ifm_data.ifi_metric, ifm->ifm_data.ifi_metric);
            PUT(tifm->ifm_data.ifi_baudrate, ifm->ifm_data.ifi_baudrate);
            PUT(tifm->ifm_data.ifi_ipackets, ifm->ifm_data.ifi_ipackets);
            PUT(tifm->ifm_data.ifi_ierrors, ifm->ifm_data.ifi_ierrors);
            PUT(tifm->ifm_data.ifi_opackets, ifm->ifm_data.ifi_opackets);
            PUT(tifm->ifm_data.ifi_oerrors, ifm->ifm_data.ifi_oerrors);
            PUT(tifm->ifm_data.ifi_collisions, ifm->ifm_data.ifi_collisions);
            PUT(tifm->ifm_data.ifi_ibytes, ifm->ifm_data.ifi_ibytes);
            PUT(tifm->ifm_data.ifi_obytes, ifm->ifm_data.ifi_obytes);
            PUT(tifm->ifm_data.ifi_imcasts, ifm->ifm_data.ifi_imcasts);
            PUT(tifm->ifm_data.ifi_omcasts, ifm->ifm_data.ifi_omcasts);
            PUT(tifm->ifm_data.ifi_iqdrops, ifm->ifm_data.ifi_iqdrops);
            PUT(tifm->ifm_data.ifi_oqdrops, ifm->ifm_data.ifi_oqdrops);
            PUT(tifm->ifm_data.ifi_noproto, ifm->ifm_data.ifi_noproto);
            PUT(tifm->ifm_data.ifi_hwassist, ifm->ifm_data.ifi_hwassist);

            /* set target msglen */
            tmsglen = ifm->ifm_len + sa_len;
            PUT(tifm->ifm_msglen, tmsglen);
            break;
        }
        case RTM_NEWADDR: {
            struct ifa_msghdrl *ifam, *tifam;
            ifam = (struct ifa_msghdrl *)bp;
            tifam = (struct ifa_msghdrl *)tp;

            rtprintf("address. index=%d, msglen=%d, len=%d, data_off=%d\n",
                     ifam->ifam_index, ifam->ifam_msglen, ifam->ifam_len,
                     ifam->ifam_data_off);

            memcpy(tifam, ifam, sizeof(*ifam));

            /* copy and repack sockaddrs */
            sa_len = host_to_target_copy_sockaddrs(ifam->ifam_addrs,
                                                   IFA_MSGHDRL_RTA(ifam),
                                                   IFA_MSGHDRL_RTA(tifam));

            /* byteswap */
            PUT(tifam->ifam_addrs, ifam->ifam_addrs);
            PUT(tifam->ifam_flags, ifam->ifam_flags);
            PUT(tifam->ifam_index, ifam->ifam_index);
            PUT(tifam->ifam_len, ifam->ifam_len);
            PUT(tifam->ifam_data_off, ifam->ifam_data_off);
            PUT(tifam->ifam_metric, ifam->ifam_metric);
            PUT(tifam->ifam_data.ifi_datalen, ifam->ifam_data.ifi_datalen);
            PUT(tifam->ifam_data.ifi_mtu, ifam->ifam_data.ifi_mtu);
            PUT(tifam->ifam_data.ifi_metric, ifam->ifam_data.ifi_metric);
            PUT(tifam->ifam_data.ifi_baudrate, ifam->ifam_data.ifi_baudrate);
            PUT(tifam->ifam_data.ifi_ipackets, ifam->ifam_data.ifi_ipackets);
            PUT(tifam->ifam_data.ifi_ierrors, ifam->ifam_data.ifi_ierrors);
            PUT(tifam->ifam_data.ifi_opackets, ifam->ifam_data.ifi_opackets);
            PUT(tifam->ifam_data.ifi_oerrors, ifam->ifam_data.ifi_oerrors);
            PUT(tifam->ifam_data.ifi_collisions, ifam->ifam_data.ifi_collisions);
            PUT(tifam->ifam_data.ifi_ibytes, ifam->ifam_data.ifi_ibytes);
            PUT(tifam->ifam_data.ifi_obytes, ifam->ifam_data.ifi_obytes);
            PUT(tifam->ifam_data.ifi_imcasts, ifam->ifam_data.ifi_imcasts);
            PUT(tifam->ifam_data.ifi_omcasts, ifam->ifam_data.ifi_omcasts);
            PUT(tifam->ifam_data.ifi_iqdrops, ifam->ifam_data.ifi_iqdrops);
            PUT(tifam->ifam_data.ifi_oqdrops, ifam->ifam_data.ifi_oqdrops);
            PUT(tifam->ifam_data.ifi_noproto, ifam->ifam_data.ifi_noproto);
            PUT(tifam->ifam_data.ifi_hwassist, ifam->ifam_data.ifi_hwassist);

            /* set target msglen */
            tmsglen = ifam->ifam_len + sa_len;
            PUT(tifam->ifam_msglen, tmsglen);

            break;
        }

        default:
            /*
             * can't happen, since the kernel only generates RTM_IFINFO and
             * RTM_NEWADDR messages for this sysctl, but if it does, don't
             * copy the message.
             */
            tmsglen = 0;
            break;
        }

        rtprintf("after, msglen = %d\n", tmsglen);
        hexdump("t", trtm, tmsglen);

        bp += rtm->rtm_msglen;
        tp += tmsglen;
    }

    len = tp - tbuf;
    memcpy(buf, tbuf, len);
    *tlen = len;

    rtprintf("in sysctl, tlen = %zd\n", len);
    hexdump("z", buf, len);
    fflush(stdout);

    return ret;
#else
    abort();
#endif
}

/*
 * The following should maybe go some place else.  Also, see the note about
 * using "thunk" for sysctl's that pass data using structures.
 */
/* From sys/mount.h: */
struct target_xvfsconf {
    abi_ulong vfc_vfsops;       /* filesystem op vector - not used */
    char vfc_name[16];          /* filesystem type name */
    int32_t  vfc_typenum;       /* historic fs type number */
    int32_t  vfc_refcount;      /* number mounted of this type */
    int32_t  vfc_flags;         /* permanent flags */
    abi_ulong vfc_next;         /* next int list - not used */
};

/* vfc_flag definitions */
#define TARGET_VFCF_STATIC     0x00010000  /* statically compiled into kernel */
#define TARGET_VFCF_NETWORK    0x00020000  /* may get data over the network */
#define TARGET_VFCF_READONLY   0x00040000  /* writes are not implemented */
#define TARGET_VFCF_SYNTHETIC  0x00080000  /* doesn't represent real files */
#define TARGET_VFCF_LOOPBACK   0x00100000  /* aliases some other mounted FS */
#define TARGET_VFCF_UNICODE    0x00200000  /* stores file names as Unicode */
#define TARGET_VFCF_JAIL       0x00400000  /* can be mounted within a jail */
#define TARGET_VFCF_DELEGADMIN 0x00800000  /* supports delegated admin */
#define TARGET_VFCF_SBDRY      0x01000000  /* defer stop requests */

#if !defined(__linux__)
static int
host_to_target_vfc_flags(int flags)
{
    int ret = 0;

    if (flags & VFCF_STATIC) {
        ret |= TARGET_VFCF_STATIC;
    }
    if (flags & VFCF_NETWORK) {
        ret |= TARGET_VFCF_NETWORK;
    }
    if (flags & VFCF_READONLY) {
        ret |= TARGET_VFCF_READONLY;
    }
    if (flags & VFCF_SYNTHETIC) {
        ret |= TARGET_VFCF_SYNTHETIC;
    }
    if (flags & VFCF_LOOPBACK) {
        ret |= TARGET_VFCF_LOOPBACK;
    }
    if (flags & VFCF_UNICODE) {
        ret |= TARGET_VFCF_UNICODE;
    }
    if (flags & VFCF_JAIL) {
        ret |= TARGET_VFCF_JAIL;
    }
    if (flags & VFCF_DELEGADMIN) {
        ret |= TARGET_VFCF_DELEGADMIN;
    }
#ifdef VFCF_SBRDY
    if (flags & VFCF_SBDRY) {
        ret |= TARGET_VFCF_SBDRY;
    }
#endif
    return ret;
}
#endif

/*
 * This uses the undocumented oidfmt interface to find the kind of a requested
 * sysctl, see /sys/kern/kern_sysctl.c:sysctl_sysctl_oidfmt() (compare to
 * src/sbin/sysctl/sysctl.c)
 */
static int oidfmt(int *oid, int len, char *fmt, uint32_t *kind)
{
#if !defined(__linux__)
    int qoid[CTL_MAXNAME + 2];
    uint8_t buf[BUFSIZ];
    int i;
    size_t j;

    qoid[0] = CTL_SYSCTL;
    qoid[1] = CTL_SYSCTL_OIDFMT;
    memcpy(qoid + 2, oid, len * sizeof(int));

    j = sizeof(buf);
    i = sysctl(qoid, len + 2, buf, &j, 0, 0);
    if (i) {
        return i;
    }

    if (kind) {
        *kind = *(uint32_t *)buf;
    }

    if (fmt) {
        strcpy(fmt, (char *)(buf + sizeof(uint32_t)));
    }
    return 0;
#else
    return -TARGET_ENOENT;
#endif
}

/*
 * Convert the old value from host to guest.
 *
 * For LONG and ULONG on ABI32, we need to 'down convert' the 8 byte quantities
 * to 4 bytes. The caller setup a buffer in host memory to get this data from
 * the kernel and pass it to us. We do the down conversion and adjust the length
 * so the caller knows what to write as the returned length into the target when
 * it copies the down converted values into the target.
 *
 * For normal integral types, we just need to byte swap. No size changes.
 *
 * For strings and node data, there's no conversion needed.
 *
 * For opaque data, per sysctl OID converts take care of it.
 */
static void h2g_old_sysctl(void *holdp, size_t *holdlen, uint32_t kind)
{
    size_t len;
    int hlen, glen;
    uint8_t *hp, *gp;

    /*
     * Although rare, we can have arrays of sysctl. Both sysctl_old_ddb in
     * kern_sysctl.c and show_var in sbin/sysctl/sysctl.c have code that loops
     * this way.  *holdlen has been set by the kernel to the host's length.
     * Only LONG and ULONG on ABI32 have different sizes: see below.
     */
    gp = hp = (uint8_t *)holdp;
    len = 0;
    hlen = host_ctl_size[kind & CTLTYPE];
    glen = guest_ctl_size[kind & CTLTYPE];

    /*
     * hlen == 0 for CTLTYPE_STRING and CTLTYPE_NODE, which need no conversion
     * as well as CTLTYPE_OPAQUE, which needs special converters.
     */
    if (hlen == 0) {
        return;
    }

    while (len < *holdlen) {
        if (hlen == glen) {
            switch (hlen) {
            case 1:
                /* Nothing needed: no byteswapping and assigning in place */
                break;
            case 2:
                *(uint16_t *)gp = tswap16(*(uint16_t *)hp);
                break;
            case 4:
                *(uint32_t *)gp = tswap32(*(uint32_t *)hp);
                break;
            case 8:
                *(uint64_t *)gp = tswap64(*(uint64_t *)hp);
                break;
            default:
                g_assert_not_reached();
            }
        } else {
#ifdef TARGET_ABI32
            /*
             * Saturating assignment for the only two types that differ between
             * 32-bit and 64-bit machines. All other integral types have the
             * same, fixed size and will be converted w/o loss of precision
             * in the above switch.
             */
            switch (kind & CTLTYPE) {
            case CTLTYPE_LONG:
                *(abi_long *)gp = tswap32(h2g_long_sat(*(long *)hp));
                break;
            case CTLTYPE_ULONG:
                *(abi_ulong *)gp = tswap32(h2g_ulong_sat(*(u_long *)hp));
                break;
            default:
                g_assert_not_reached();
            }
#else
            g_assert_not_reached();
#endif
        }
        gp += glen;
        hp += hlen;
        len += hlen;
    }
#ifdef TARGET_ABI32
    if (hlen != glen) {
        *holdlen = (*holdlen / hlen) * glen;
    }
#endif
}

/*
 * Convert the undocmented name2oid sysctl data for the target.
 */
static inline void sysctl_name2oid(uint32_t *holdp, size_t holdlen)
{
    size_t i, num = holdlen / sizeof(uint32_t);

    for (i = 0; i < num; i++) {
        holdp[i] = tswap32(holdp[i]);
    }
}

static inline void sysctl_oidfmt(uint32_t *holdp)
{
    /* byte swap the kind */
    holdp[0] = tswap32(holdp[0]);
}

#if defined(__linux__)
static inline int sysctlnametomib(const char *name, int *mibp, size_t *sizep)
{
	abort();
}

static inline int sysctlbyname(const char *name, void *oldp, size_t *oldlenp,
         const void *newp, size_t newlen)
{
	abort();
}

static inline int sysctl(const int *name, u_int namelen, void *oldp, size_t *oldlenp,
	const void *newp, size_t newlen)
{
	return -TARGET_ENOENT;
}
#endif

static abi_long do_freebsd_sysctl_oid(CPUArchState *env, int32_t *snamep,
        int32_t namelen, void *holdp, size_t *holdlenp, void *hnewp,
        size_t newlen)
{
    uint32_t kind = 0;
    abi_long ret;
    size_t holdlen, oldlen;
#ifdef TARGET_ABI32
    void *old_holdp;
#endif
    CPUState *cpu = env_cpu(env);
    TaskState *ts = (TaskState *)cpu->opaque;
    const char *ret_str;

    holdlen = oldlen = *holdlenp;
    oidfmt(snamep, namelen, NULL, &kind);

    /* Handle some arch/emulator dependent sysctl()'s here. */
    switch (snamep[0]) {
#if defined(TARGET_PPC) || defined(TARGET_PPC64)
    case CTL_MACHDEP:
        switch (snamep[1]) {
        case 1:    /* CPU_CACHELINE */
            holdlen = sizeof(abi_uint);
            (*(abi_uint *)holdp) = tswap32(env->dcache_line_size);
            ret = 0;
            goto out;
        }
        break;
#endif
    case CTL_KERN:
        switch (snamep[1]) {
#if defined(__linux__)
        case KERN_OSTYPE:
            holdlen = sizeof(FREEBSD_ABI_VENDOR);
            ret_str = FREEBSD_ABI_VENDOR;
	    goto out_str;

	case KERN_OSRELEASE:
            holdlen = strlen(osrelease_str) + 1;
            ret_str = osrelease_str;
            goto out_str;

        case KERN_VERSION:
            holdlen = strlen(kversion_str) + 1;
            ret_str = kversion_str;
            goto out_str;

	case KERN_OSRELDATE:
	    if (oldlen) {
                (*(abi_uint *)holdp) = tswapal(__FreeBSD_version);
	    }
	    holdlen = sizeof(abi_uint);
	    ret = 0;
	    goto out;

	case KERN_HOSTNAME:
            do {
                size_t nlen = sysconf(_SC_HOST_NAME_MAX);
                char *name = alloca(sysconf(_SC_HOST_NAME_MAX));
                if (name == NULL)
                    break;
                ret = gethostname(name, nlen);
                if (ret != 0)
                    break;
                ret_str = name;
                holdlen = strlen(name) + 1;
                goto out_str;
            } while (0);
            holdlen = 0;
            ret = -TARGET_ENOMEM;
            goto out;

        case KERN_ARND:
            if (oldlen) {
                (*(abi_ulong *)holdp) = tswapal(0x42434445);
            }
            holdlen = sizeof(abi_ulong);
            ret = 0;
            goto out;

        case KERN_ARGMAX:
            if (oldlen) {
                (*(abi_uint *)holdp) = tswapal(sysconf(_SC_ARG_MAX));
            }
            holdlen = sizeof(abi_uint);
            ret = 0;
            goto out;
#endif

        case KERN_USRSTACK:
            if (oldlen) {
                (*(abi_ulong *)holdp) = tswapal(TARGET_USRSTACK);
            }
            holdlen = sizeof(abi_ulong);
            ret = 0;
            goto out;

        case KERN_PS_STRINGS:
            if (oldlen) {
                (*(abi_ulong *)holdp) = tswapal(TARGET_PS_STRINGS);
            }
            holdlen = sizeof(abi_ulong);
            ret = 0;
            goto out;

        case KERN_PROC:
            switch (snamep[2]) {
            case KERN_PROC_PATHNAME:
                holdlen = strlen(ts->bprm->fullpath) + 1;
                ret = 0;
                if (holdp) {
                    if (oldlen < holdlen) {
                        ret = -TARGET_ENOMEM;
                        holdlen = oldlen;
                    }
                    memcpy(holdp, ts->bprm->fullpath, holdlen);
                }
                goto out;

            case KERN_PROC_ALL:
            case KERN_PROC_PROC:
            case KERN_PROC_PID:
            case KERN_PROC_PID | KERN_PROC_INC_THREAD:
            case KERN_PROC_PGRP:
            case KERN_PROC_PGRP | KERN_PROC_INC_THREAD:
            case KERN_PROC_SESSION:
            case KERN_PROC_SESSION | KERN_PROC_INC_THREAD:
            case KERN_PROC_TTY:
            case KERN_PROC_TTY | KERN_PROC_INC_THREAD:
            case KERN_PROC_UID:
            case KERN_PROC_UID | KERN_PROC_INC_THREAD:
            case KERN_PROC_RUID:
            case KERN_PROC_RUID | KERN_PROC_INC_THREAD:
                ret = do_sysctl_kern_getprocs(snamep[2], snamep[3], oldlen,
                                              holdp, &holdlen);
                goto out;

            case KERN_PROC_FILEDESC:
                ret = do_sysctl_kern_proc_filedesc(snamep[3], oldlen, holdp,
                                                   &holdlen);
                goto out;

            case KERN_PROC_VMMAP:
                ret = do_sysctl_kern_proc_vmmap(snamep[3], oldlen, holdp,
                                                &holdlen);
                goto out;

            default:
                break;
            }
            break;

        default:
            break;
        }
        break;

    case CTL_VFS:
    {
#if !defined(__linux__)
        static int oid_vfs_conflist;

        if (!oid_vfs_conflist) {
            int real_oid[CTL_MAXNAME+2];
            size_t len = sizeof(real_oid) / sizeof(int);

            if (sysctlnametomib("vfs.conflist", real_oid, &len) >= 0) {
                oid_vfs_conflist = real_oid[1];
            }
        }

        if (oid_vfs_conflist && snamep[1] == oid_vfs_conflist) {
            struct xvfsconf *xvfsp;
            struct target_xvfsconf *txp;
            int cnt, i;

            if (sysctlbyname("vfs.conflist", NULL, &holdlen, NULL, 0) < 0) {
                ret = -1;
                goto out;
            }
            if (!holdp) {
                ret = 0;
                goto out;
            }
            xvfsp = g_malloc(holdlen);
            if (xvfsp == NULL) {
                ret = -TARGET_ENOMEM;
                goto out;
            }
            if (sysctlbyname("vfs.conflist", xvfsp, &holdlen, NULL, 0) < 0) {
                g_free(xvfsp);
                ret = -1;
                goto out;
            }
            cnt = holdlen / sizeof(struct xvfsconf);
            holdlen = cnt * sizeof(struct target_xvfsconf);
            txp = (struct target_xvfsconf *)holdp;
            for (i = 0; i < cnt; i++) {
                txp[i].vfc_vfsops = 0;
                strlcpy(txp[i].vfc_name, xvfsp[i].vfc_name,
                        sizeof(txp[i].vfc_name));
                txp[i].vfc_typenum = tswap32(xvfsp[i].vfc_typenum);
                txp[i].vfc_refcount = tswap32(xvfsp[i].vfc_refcount);
                txp[i].vfc_flags = tswap32(
                    host_to_target_vfc_flags(xvfsp[i].vfc_flags));
                txp[i].vfc_next = 0;
            }
            g_free(xvfsp);
            ret = 0;
            goto out;
        }
#else
	abort();
#endif
    }
    break;

    case CTL_HW:
        switch (snamep[1]) {
        case HW_MACHINE:
            holdlen = sizeof(TARGET_HW_MACHINE);
            if (holdp) {
                strlcpy(holdp, TARGET_HW_MACHINE, oldlen);
            }
            ret = 0;
            goto out;

        case HW_MACHINE_ARCH:
        {
#ifdef	TARGET_ARM
            /*
             * For ARM, we'll try to grab the MACHINE_ARCH from the ELF that we are
             * currently emulating. It should have a .note containing the
             * MACHINE_ARCH that this binary has been compiled for, so we'll assume
             * this is right and otherwise matches the jail/environment we're
             * operating out of.
             *
             * If we can't find the expected ARCH tag, fallback to armv7 as a
             * sensible default.
             */
            char *machine_arch;
            if (get_target_arch(ts, &machine_arch) == 0) {
                holdlen = strlen(machine_arch);
                if (holdp) {
                    strlcpy(holdp, machine_arch, oldlen);
                }
                free(machine_arch);
                ret = 0;
                goto out;
            }
#endif
            holdlen = sizeof(TARGET_HW_MACHINE_ARCH);
            if (holdp) {
                strlcpy(holdp, TARGET_HW_MACHINE_ARCH, oldlen);
            }
            ret = 0;
            goto out;
        }
        case HW_NCPU:
            if (oldlen) {
                (*(abi_int *)holdp) = tswap32(bsd_get_ncpu());
            }
            holdlen = sizeof(int32_t);
            ret = 0;
            goto out;
#if defined(TARGET_ARM)
        case HW_FLOATINGPT:
            if (oldlen) {
                ARMCPU *cpu2 = env_archcpu(env);
                *(abi_int *)holdp = cpu_isar_feature(aa32_vfp, cpu2);
            }
            holdlen = sizeof(abi_int);
            ret = 0;
            goto out;
#endif


#ifdef TARGET_ABI32
        case HW_PHYSMEM:
        case HW_USERMEM:
        case HW_REALMEM:
            holdlen = sizeof(abi_ulong);
            ret = 0;

            if (oldlen) {
                int mib[2] = {snamep[0], snamep[1]};
                unsigned long lvalue;
                size_t len = sizeof(lvalue);

                if (sysctl(mib, 2, &lvalue, &len, NULL, 0) == -1) {
                    ret = -1;
                } else {
                    lvalue = cap_memory(lvalue);
                    (*(abi_ulong *)holdp) = tswapal((abi_ulong)lvalue);
                }
            }
            goto out;
#endif

        default:
        {
            static int oid_hw_availpages;
            static int oid_hw_pagesizes;

            if (!oid_hw_availpages) {
                int real_oid[CTL_MAXNAME + 2];
                size_t len = sizeof(real_oid) / sizeof(int);

                if (sysctlnametomib("hw.availpages", real_oid, &len) >= 0) {
                    oid_hw_availpages = real_oid[1];
                }
            }
            if (!oid_hw_pagesizes) {
                int real_oid[CTL_MAXNAME + 2];
                size_t len = sizeof(real_oid) / sizeof(int);

                if (sysctlnametomib("hw.pagesizes", real_oid, &len) >= 0) {
                    oid_hw_pagesizes = real_oid[1];
                }
            }

            if (oid_hw_availpages && snamep[1] == oid_hw_availpages) {
                long lvalue;
                size_t len = sizeof(lvalue);

                if (sysctlbyname("hw.availpages", &lvalue, &len, NULL, 0) == -1) {
                    ret = -1;
                } else {
                    if (oldlen) {
                        lvalue = scale_to_guest_pages(lvalue);
                        (*(abi_ulong *)holdp) = tswapal((abi_ulong)lvalue);
                    }
                    holdlen = sizeof(abi_ulong);
                    ret = 0;
                }
                goto out;
            }

            if (oid_hw_pagesizes && snamep[1] == oid_hw_pagesizes) {
                if (oldlen) {
                    (*(abi_ulong *)holdp) = tswapal((abi_ulong)TARGET_PAGE_SIZE);
                    ((abi_ulong *)holdp)[1] = 0;
                }
                holdlen = sizeof(abi_ulong) * 2;
                ret = 0;
                goto out;
            }
            break;
        }
        }
        break;

    case CTL_NET:
        if (snamep[1] == PF_ROUTE && snamep[2] == 0 && snamep[3] == 0 &&
            snamep[4] == NET_RT_IFLISTL && snamep[5] == 0) {
            ret = do_sysctl_net_routetable_iflistl(snamep, namelen, oldlen,
                                                   holdp, &holdlen);
            goto out;
        }
        break;

    default:
        break;
    }

#ifdef TARGET_ABI32
    /*
     * For long and ulong with a 64-bit host and a 32-bit target we have to do
     * special things. holdlen here is the length provided by the target to the
     * system call. So we allocate a buffer twice as large because longs are
     * twice as big on the host which will be writing them. In h2g_old_sysctl
     * we'll adjust them and adjust the length.
     */
    if (kind == CTLTYPE_LONG || kind == CTLTYPE_ULONG) {
        old_holdp = holdp;
        holdlen = holdlen * 2;
        holdp = g_malloc(holdlen);
    }
#endif

    ret = get_errno(sysctl(snamep, namelen, holdp, &holdlen, hnewp, newlen));
    if (!ret && (holdp != 0)) {

        /*
         * In case we are retrieving the argument vectors with kvm_getargv,
         * remove the interpreter name and the interpreter prefix from the
         * argument vectors
         */
        if (snamep[0] == CTL_KERN && snamep[1] == KERN_PROC &&
            snamep[2] == KERN_PROC_ARGS) {
            int len;

            /*
             * remove the interpreter name (1st arg:
             * /usr/local/bin/qemu-xxx-static)
             */
            len = strlen(holdp);
            memcpy(holdp, holdp + len + 1, holdlen - len - 1);
            holdlen = holdlen - len - 1;

            /*
             * if the 2nd arg is "-L" remove it and remove the 3rd arg
             * (interpreter prefix: /usr/gnemul/qemu-xxx)
             */
            if (strncmp(holdp, "-L", 2) == 0) {
                memcpy(holdp, holdp + 3, holdlen - 3);
                holdlen = holdlen - 3;

                len = strlen(holdp);
                memcpy(holdp, holdp + len + 1, holdlen - len - 1);
                holdlen = holdlen - len - 1;
            }
        }

        if (snamep[0] == CTL_SYSCTL) {
            switch (snamep[1]) {
            case CTL_SYSCTL_NEXT:
            case CTL_SYSCTL_NAME2OID:
            case CTL_SYSCTL_NEXTNOSKIP:
                /*
                 * All of these return an OID array, so we need to convert to
                 * target.
                 */
                sysctl_name2oid(holdp, holdlen);
                break;

            case CTL_SYSCTL_OIDFMT:
                /* Handle oidfmt */
                sysctl_oidfmt(holdp);
                break;
            case CTL_SYSCTL_OIDDESCR:
            case CTL_SYSCTL_OIDLABEL:
            default:
                /* Handle it based on the type */
                h2g_old_sysctl(holdp, &holdlen, kind);
                /* NB: None of these are LONG or ULONG */
                break;
            }
        } else {
            /*
             * Need to convert from host to target. All the weird special cases
             * are handled above.
             */
            h2g_old_sysctl(holdp, &holdlen, kind);
#ifdef TARGET_ABI32
            /*
             * For the 32-bit on 64-bit case, for longs we need to copy the
             * now-converted buffer to the target and free the buffer.
             */
            if (kind == CTLTYPE_LONG || kind == CTLTYPE_ULONG) {
                memcpy(old_holdp, holdp, holdlen);
                g_free(holdp);
                holdp = old_holdp;
            }
#endif
        }
    }
#ifdef DEBUG
    else {
        printf("sysctl(mib[0]=%d, mib[1]=%d, mib[3]=%d...) returned %d\n",
               snamep[0], snamep[1], snamep[2], (int)ret);
    }
#endif

out:
    *holdlenp = holdlen;
    return ret;
out_str:
    ret = 0;
    if (oldlen) {
        if (oldlen < holdlen) {
            ret = -TARGET_ENOMEM;
            holdlen = oldlen;
        }
        memcpy(holdp, ret_str, holdlen);
    }
    goto out;
}


struct handler {
    const char *name;
    int (*func)(void *holdp, size_t oldlen, size_t *holdlen);
};

// Handlers for each case
static int
handle_hw_pagesizes(void *holdp, size_t oldlen, size_t *holdlen)
{
    *holdlen = sizeof(pagesizes);
    if (oldlen) {
        if (*holdlen > oldlen)
            *holdlen = oldlen;
        memcpy(holdp, pagesizes, *holdlen);
    }
    return 0; // success
}

static int
handle_hw_availpages(void *holdp, size_t oldlen, size_t *holdlen)
{
    *holdlen = sizeof(abi_long);
    if (oldlen) {
        struct meminfo_entry se = get_meminfo_value("MemAvailable");
        if (se.error) {
            return -TARGET_EINVAL;
        }
        *((abi_long *)holdp) = se.value / PAGE_SIZE;
    }
    return 0; // success
}

static int
handle_kern_ps_strings(void *holdp, size_t oldlen, size_t *holdlen)
{
    return -TARGET_EINVAL;
}

static int
handle_hw_machine_arch(void *holdp, size_t oldlen, size_t *holdlen)
{
    struct utsname buffer;
    if (uname(&buffer) == -1) {
        return -TARGET_EINVAL;
    }
    *holdlen = strlen(buffer.machine) + 1;
    if (oldlen) {
        if (*holdlen > oldlen)
            *holdlen = oldlen;
        memcpy(holdp, buffer.machine, *holdlen);
    }
    return 0;
}

static const struct handler handlers[] = {
    {"hw.pagesizes", handle_hw_pagesizes},
    {"hw.availpages", handle_hw_availpages},
    {"kern.ps_strings", handle_kern_ps_strings},
    {"hw.machine_arch", handle_hw_machine_arch},
    {NULL, NULL} // Terminator
};

/*
 * This syscall was created to make sysctlbyname(3) more efficient, but we can't
 * really provide it in bsd-user.  Notably, we must always translate the names
 * independently since some sysctl values have to be faked for the target
 * environment, so it still has to break down to two syscalls for the underlying
 * implementation.
 */
abi_long do_freebsd_sysctlbyname(CPUArchState *env, abi_ulong namep,
        int32_t namelen, abi_ulong oldp, abi_ulong oldlenp, abi_ulong newp,
        abi_ulong newlen)
{
    abi_long ret = -TARGET_EFAULT;
    void *holdp = NULL, *hnewp = NULL;
    char *snamep = NULL;
    int oid[CTL_MAXNAME + 2];
    size_t holdlen, oidplen;
    abi_ulong oldlen = 0;

    /* oldlenp is read/write, pre-check here for write */
    if (oldlenp) {
        if (!access_ok(VERIFY_WRITE, oldlenp, sizeof(abi_ulong)) ||
            get_user_ual(oldlen, oldlenp)) {
            goto out;
        }
    }
    snamep = lock_user_string(namep);
    if (snamep == NULL) {
        goto out;
    }
    if (newp) {
        hnewp = lock_user(VERIFY_READ, newp, newlen, 1);
        if (hnewp == NULL) {
            goto out;
        }
    }
    if (oldp) {
        holdp = lock_user(VERIFY_WRITE, oldp, oldlen, 0);
        if (holdp == NULL) {
            goto out;
        }
    }
    holdlen = oldlen;

    for (const struct handler *h = handlers; h->name; h++) {
        if (namelen == strlen(h->name) && strcmp(snamep, h->name) == 0) {
            ret = h->func(holdp, oldlen, &holdlen);
	    goto out;
        }
    }

    oidplen = ARRAY_SIZE(oid);
    if (sysctlnametomib(snamep, oid, &oidplen) != 0) {
        ret = -TARGET_EINVAL;
        goto out;
    }

    ret = do_freebsd_sysctl_oid(env, oid, oidplen, holdp, &holdlen, hnewp,
        newlen);

    /*
     * writeability pre-checked above. __sysctl(2) returns ENOMEM and updates
     * oldlenp for the proper size to use.
     */
    if (oldlenp && (ret == 0 || ret == -TARGET_ENOMEM)) {
        put_user_ual(holdlen, oldlenp);
    }
out:
    unlock_user(snamep, namep, 0);
    unlock_user(holdp, oldp, ret == 0 ? holdlen : 0);
    unlock_user(hnewp, newp, 0);

    return ret;
}

abi_long do_freebsd_sysctl(CPUArchState *env, abi_ulong namep, int32_t namelen,
        abi_ulong oldp, abi_ulong oldlenp, abi_ulong newp, abi_ulong newlen)
{
    abi_long ret = -TARGET_EFAULT;
    void *hnamep, *holdp = NULL, *hnewp = NULL;
    size_t holdlen;
    abi_ulong oldlen = 0;
    int32_t *snamep = g_malloc(sizeof(int32_t) * namelen), *p, *q, i;

    /* oldlenp is read/write, pre-check here for write */
    if (oldlenp) {
        if (!access_ok(VERIFY_WRITE, oldlenp, sizeof(abi_ulong)) ||
            get_user_ual(oldlen, oldlenp)) {
            goto out;
        }
    }
    hnamep = lock_user(VERIFY_READ, namep, namelen, 1);
    if (hnamep == NULL) {
        goto out;
    }
    if (newp) {
        hnewp = lock_user(VERIFY_READ, newp, newlen, 1);
        if (hnewp == NULL) {
            goto out;
        }
    }
    if (oldp) {
        holdp = lock_user(VERIFY_WRITE, oldp, oldlen, 0);
        if (holdp == NULL) {
            goto out;
        }
    }
    holdlen = oldlen;
    for (p = hnamep, q = snamep, i = 0; i < namelen; p++, i++, q++) {
        *q = tswap32(*p);
    }

    ret = do_freebsd_sysctl_oid(env, snamep, namelen, holdp, &holdlen, hnewp,
        newlen);

    /*
     * writeability pre-checked above. __sysctl(2) returns ENOMEM and updates
     * oldlenp for the proper size to use.
     */
    int is_nomem = (ret == -TARGET_ENOMEM);
    if (oldlenp && (ret == 0 || is_nomem)) {
        put_user_ual(holdlen, oldlenp);
    }
    unlock_user(hnamep, namep, 0);
    unlock_user(holdp, oldp, (ret == 0 || is_nomem) ? holdlen : 0);
out:
    g_free(snamep);
    return ret;
}

/* sysarch() is architecture dependent. */
abi_long do_freebsd_sysarch(void *cpu_env, abi_long arg1, abi_long arg2)
{
    return do_freebsd_arch_sysarch(cpu_env, arg1, arg2);
}
