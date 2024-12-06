/*
 *  FreeBSD conversion extern declarations
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

#ifndef QEMU_OS_H
#define QEMU_OS_H

/* qemu/osdep.h pulls in the rest */

#if 0
#include <sys/acl.h>
#endif
#include <sys/mount.h>
#include <sys/timex.h>
#if 0
#include <sys/rtprio.h>
#endif
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/termios.h>
#include <netinet/in.h>

#undef ifr_name
#undef SIGRTMAX
#define SIGRTMAX __SIGRTMAX
#undef SIGRTMIN
#define SIGRTMIN __SIGRTMIN
#define TARGET_STRERR_FILENO 2
#define __lwpid_t int32_t
#define __uintptr_t uintptr_t

#include "target_os_defs.h"

struct freebsd11_stat;
struct freebsd11_statfs;
struct statfs;
struct rtprio;

typedef struct fsid { int32_t val[2]; } freebsd_fsid_t; /* filesystem id type */

#define MAXFIDSZ        16
#define MAXCOMLEN       19              /* max command name remembered */

struct fid {
	u_short         fid_len;                /* length of data in bytes */
	u_short         fid_data0;              /* force longword alignment */
	char            fid_data[MAXFIDSZ];     /* data (variable length) */
};

struct fhandle {
	freebsd_fsid_t	fh_fsid;        /* Filesystem id of mount point */
	struct		fid fh_fid;     /* Filesys specific id */
};
typedef struct fhandle  fhandle_t;

#define	rounddown(x, y)	(((x)/(y))*(y))
#define __builtin_align_up(x, align)    \
	((__typeof__(x))(((__intptr_t)(x)+((align)-1))&(~((align)-1))))
#define __align_up(x, y) __builtin_align_up(x, y)
#define	__unused	__attribute__((__unused__))
#define	roundup2(x, y)  __align_up(x, y) /* if y is powers of two */
#define __sym_compat(sym,impl,verid)    \
        __asm__(".symver " #impl ", " #sym "@" #verid)
#define        nitems(x)       (sizeof((x)) / sizeof((x)[0]))

#define PAGE_SHIFT 12
#define PAGE_SIZE  (1<<PAGE_SHIFT)
#define PAGE_MASK  (PAGE_SIZE-1)

typedef       __uint32_t      fflags_t;

struct freebsd11_stat {
        __uint32_t st_dev;              /* inode's device */
        __uint32_t st_ino;              /* inode's number */
        mode_t    st_mode;              /* inode protection mode */
        __uint16_t st_nlink;            /* number of hard links */
        uid_t     st_uid;               /* user ID of the file's owner */
        gid_t     st_gid;               /* group ID of the file's group */
        __uint32_t st_rdev;             /* device type */
        struct  timespec st_atim;       /* time of last access */
        struct  timespec st_mtim;       /* time of last data modification */
        struct  timespec st_ctim;       /* time of last file status change */
        off_t     st_size;              /* file size, in bytes */
        blkcnt_t st_blocks;             /* blocks allocated for file */
        blksize_t st_blksize;           /* optimal blocksize for I/O */
        fflags_t  st_flags;             /* user defined flags for file */
        __uint32_t st_gen;              /* file generation number */
        __int32_t st_lspare;
        struct timespec st_birthtim;    /* time of file creation */
        /*
         * Explicitly pad st_birthtim to 16 bytes so that the size of
         * struct stat is backwards compatible.  We use bitfields instead
         * of an array of chars so that this doesn't require a C99 compiler
         * to compile if the size of the padding is 0.  We use 2 bitfields
         * to cover up to 64 bits on 32-bit machines.  We assume that
         * CHAR_BIT is 8...
         */
        unsigned int :(8 / 2) * (16 - (int)sizeof(struct timespec));
        unsigned int :(8 / 2) * (16 - (int)sizeof(struct timespec));
};

/* os-time.c */
abi_long t2h_freebsd_timeval(struct timeval *tv, abi_ulong target_tv_addr);
abi_long h2t_freebsd_timeval(struct timeval *tv, abi_ulong target_tv_addr);

abi_long t2h_freebsd_timespec(struct timespec *ts, abi_ulong target_ts_addr);
abi_long h2t_freebsd_timespec(abi_ulong target_ts_addr, struct timespec *ts);

abi_long t2h_freebsd_umtx_time(abi_ulong target_ut_addr,
	abi_ulong target_ut_size, void *host_t, size_t *host_tsz);

abi_long t2h_freebsd_timex(struct timex *host_tx, abi_ulong target_tx_addr);

abi_long h2t_freebsd_ntptimeval(abi_ulong target_ntv_addr,
        struct ntptimeval *ntv);

abi_ulong copy_from_user_fdset(fd_set *fds, abi_ulong target_fds_addr, int n);
abi_ulong copy_from_user_fdset_ptr(fd_set *fds, fd_set **fds_ptr,
        abi_ulong target_fds_addr, int n);
abi_long copy_to_user_fdset(abi_ulong target_fds_addr, const fd_set *fds,
        int n);

abi_int next_free_host_timer(void);
int host_to_target_timerid(int timerid);
abi_long target_to_host_itimerspec(struct itimerspec *host_itspec,
        abi_ulong target_addr);
abi_long host_to_target_itimerspec(abi_ulong target_addr,
        struct itimerspec *host_its);
int get_timer_id(abi_long arg);

/* os-socket.c */
abi_long t2h_freebsd_cmsg(struct msghdr *msgh,
                struct target_msghdr *target_msgh);
abi_long h2t_freebsd_cmsg(struct target_msghdr *target_msgh,
                struct msghdr *msgh);

/* os-stat.c */
abi_long h2t_freebsd11_stat(abi_ulong target_addr,
        struct stat *host_st);
abi_long h2t_freebsd11_nstat(abi_ulong target_addr,
        struct freebsd11_stat *host_st);
abi_long t2h_freebsd_fhandle(fhandle_t *host_fh, abi_ulong target_addr);
abi_long h2t_freebsd_fhandle(abi_ulong target_addr, fhandle_t *host_fh);
abi_long h2t_freebsd11_statfs(abi_ulong target_addr,
    struct freebsd11_statfs *host_statfs);
abi_long target_to_host_fcntl_cmd(int cmd);
abi_long h2t_freebsd_stat(abi_ulong target_addr,
        struct stat *host_st);
abi_long h2t_freebsd_statfs(abi_ulong target_addr,
    struct statfs *host_statfs);

/* os-thread.c */
int t2h_freebsd_rtprio(struct sched_param *host_rtp, abi_ulong target_addr);
abi_long h2t_freebsd_rtprio(int host_policy, const struct sched_param *host_rtp,
        abi_ulong target_addr);
abi_long do_freebsd_thr_new(CPUArchState *env, abi_ulong target_param_addr,
        int32_t param_size);

/* os-extattr.c */
struct acl;
abi_long t2h_freebsd_acl(struct acl *host_acl, abi_ulong target_addr);
abi_long h2t_freebsd_acl(abi_ulong target_addr, struct acl *host_acl);
#if 0
abi_long t2h_freebsd_acl_type(acl_type_t *host_type, abi_long target_type);
#endif

void init_bsd_sysctl(void);

#endif /* QEMU_OS_H */
