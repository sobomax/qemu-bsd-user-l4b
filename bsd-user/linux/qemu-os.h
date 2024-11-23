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
#include <netinet/in.h>

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

#undef SYS_wait4
#undef SYS_select

#define	rounddown(x, y)	(((x)/(y))*(y))
#define __builtin_align_up(x, align)    \
	((__typeof__(x))(((__intptr_t)(x)+((align)-1))&(~((align)-1))))
#define __align_up(x, y) __builtin_align_up(x, y)
#define	__unused	__attribute__((__unused__))
#define	roundup2(x, y)  __align_up(x, y) /* if y is powers of two */
#define __sym_compat(sym,impl,verid)    \
        __asm__(".symver " #impl ", " #sym "@" #verid)
#define        nitems(x)       (sizeof((x)) / sizeof((x)[0]))

#define __FreeBSD_version 1401000
#define MAP_EXCL MAP_FIXED_NOREPLACE
#define MAP_ALIGNMENT_SHIFT     24
#define MAP_ALIGNED(n)   ((n) << MAP_ALIGNMENT_SHIFT)
#define MAP_ALIGNMENT_MASK      MAP_ALIGNED(0xff)
#define	SYS_freebsd11_mknod	14
#define	SYS___getcwd		326
#define      SYS_thr_suspend 442
#define	SYS_freebsd11_mknodat	498
#define      SYS__umtx_op    454
#define      SYS_kevent      560
#define      SYS_pread       475
#define      SYS_pwrite      476
#define      SYS_wait4       7
#define      SYS_select      93
#define      SYS_wait6       532
#define      SYS_pselect     522
#define INFTIM          (-1)
#define SHM_ANON                ((char *)1)
#define KENV_GET                0
#undef	IP_RECVOPTS
#undef	IP_RECVRETOPTS
#undef IP_RETOPTS
#undef IP_RECVDSTADDR
#undef IPV6_MULTICAST_HOPS
#undef IPV6_IPSEC_POLICY
#define       IP_RECVOPTS             5    /* bool; receive all IP opts w/dgram */
#define       IP_RECVDSTADDR          7    /* bool; receive IP dst addr w/dgram */
#define       IP_RECVRETOPTS          6    /* bool; receive IP opts for response */
#define       IP_RETOPTS              8    /* ip_opts; set/get IP options */
#define       IP_ONESBCAST            23   /* bool: send all-ones broadcast */
#define       IP_BINDANY              24   /* bool: allow bind to any address */
#define IPV6_PORTRANGE              14 /* int; range to choose for unspec port */
#define       IP_PORTRANGE            19   /* int; range to choose for unspec port */
#define       IP_RECVIF               20   /* bool; receive reception if w/dgram */
#define IPV6_USE_MIN_MTU    42 /* bool; send packets at the minimum MTU */
#define IPV6_PREFER_TEMPADDR        63 /* int; prefer temporary addresses */
#define     IPV6_BINDANY            64 /* bool: allow bind to any address */
#define ICMP6_FILTER                18 /* icmp6_filter; icmp6 filter */
#define IPV6_IPSEC_POLICY   28 /* struct; get/set security policy */
#define IPV6_FW_ADD         30 /* add a firewall rule to chain */
#define IPV6_FW_DEL         31 /* delete a firewall rule from chain */
#define ICMP6_FILTER                18 /* icmp6_filter; icmp6 filter */
#define IPV6_MULTICAST_HOPS 10 /* int; set/get IP6 multicast hops */
#define IPV6_FW_FLUSH               32 /* flush firewall rule chain */
#define IPV6_FW_ZERO                33 /* clear single/all firewall counter(s) */
#define IPV6_FW_GET         34 /* get entire firewall rule chain */
#define     IPV6_MSFILTER           74 /* struct __msfilterreq */
#define       SO_NOSIGPIPE    0x00000800      /* no SIGPIPE from EPIPE */
#define       SO_BINTIME      0x00002000      /* timestamp received dgram traffic */
#define       SO_LISTENINCQLEN        0x1013  /* socket's incomplete queue length */
#define UMTX_OP_SHM             25
#define PAGE_SHIFT 12
#define PAGE_SIZE  (1<<PAGE_SHIFT)
#define PAGE_MASK  (PAGE_SIZE-1)
#define UMTX_OP_WAIT_UINT       11
#define UMTX_OP_WAIT_UINT_PRIVATE       15
#define UMTX_OP_WAIT            2
#define UMTX_OP_WAKE            3
#define UMTX_OP_MUTEX_TRYLOCK   4
#define UMTX_OP_MUTEX_LOCK      5
#define UMTX_OP_MUTEX_UNLOCK    6
#define UMTX_OP_CV_WAIT         8
#define UMTX_OP_CV_SIGNAL       9
#define UMTX_OP_CV_BROADCAST    10
#define UMTX_OP_RW_RDLOCK       12
#define UMTX_OP_RW_WRLOCK       13
#define UMTX_OP_RW_UNLOCK       14
#define UMTX_OP_WAKE_PRIVATE    16
#define UMTX_OP_SEM2_WAIT       23
#define UMTX_OP_SEM2_WAKE       24
#define UMTX_OP_ROBUST_LISTS    26
#define UMTX_OP_MUTEX_WAIT      17
#define UMTX_OP_MUTEX_WAKE      18      /* deprecated */
#define UMTX_OP_SEM_WAIT        19      /* deprecated */
#define UMTX_OP_SEM_WAKE        20      /* deprecated */
#define        EJUSTRETURN     (-2)            /* don't modify regs, just return */

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
        struct freebsd11_stat *host_st);
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
abi_long t2h_freebsd_rtprio(struct rtprio *host_rtp, abi_ulong target_addr);
abi_long h2t_freebsd_rtprio(abi_ulong target_addr, struct rtprio *host_rtp);
abi_long do_freebsd_thr_new(CPUArchState *env, abi_ulong target_param_addr,
        int32_t param_size);

/* os-extattr.c */
struct acl;
abi_long t2h_freebsd_acl(struct acl *host_acl, abi_ulong target_addr);
abi_long h2t_freebsd_acl(abi_ulong target_addr, struct acl *host_acl);
#if 0
abi_long t2h_freebsd_acl_type(acl_type_t *host_type, abi_long target_type);
#endif

#endif /* QEMU_OS_H */
