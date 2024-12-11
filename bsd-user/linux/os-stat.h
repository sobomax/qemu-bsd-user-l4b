/*
 *  stat related system call shims and definitions
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

#ifndef BSD_USER_FREEBSD_OS_STAT_H
#define BSD_USER_FREEBSD_OS_STAT_H

#include <sys/statfs.h>

int freebsd11_stat(const char *path, struct freebsd11_stat *stat);
__sym_compat(stat, freebsd11_stat, FBSD_1.0);
int freebsd11_lstat(const char *path, struct freebsd11_stat *stat);
__sym_compat(lstat, freebsd11_lstat, FBSD_1.0);
int freebsd11_fstat(int fd, struct freebsd11_stat *stat);
__sym_compat(fstat, freebsd11_fstat, FBSD_1.0);
int freebsd11_fstatat(int fd, const char *path, struct freebsd11_stat *stat,
        int flag);

int freebsd11_fhstat(const fhandle_t *fhandle, struct freebsd11_stat *stat);
__sym_compat(fhstat, freebsd11_fhstat, FBSD_1.0);
int freebsd11_getfsstat(struct freebsd11_statfs *buf, long bufsize, int mode);
__sym_compat(getfsstat, freebsd11_getfsstat, FBSD_1.0);
int freebsd11_fhstatfs(const fhandle_t *fhandle, struct freebsd11_statfs * buf);
__sym_compat(fhstatfs, freebsd11_fhstatfs, FBSD_1.0);
int freebsd11_statfs(const char *path, struct freebsd11_statfs *buf);
__sym_compat(statfs, freebsd11_statfs, FBSD_1.0);
int freebsd11_fstatfs(int fd, struct freebsd11_statfs *buf);
__sym_compat(fstatfs, freebsd11_fstatfs, FBSD_1.0);

ssize_t freebsd11_getdirentries(int fd, char *buf, size_t nbytes, off_t *basep);
__sym_compat(getdirentries, freebsd11_getdirentries, FBSD_1.0);
ssize_t freebsd11_getdents(int fd, char *buf, size_t nbytes);
__sym_compat(getdents, freebsd11_getdents, FBSD_1.0);

/* undocumented nstat system calls */
int freebsd11_nstat(const char *path, struct freebsd11_stat *sb);
__sym_compat(nstat, freebsd11_nstat, FBSD_1.0);
int freebsd11_nlstat(const char *path, struct freebsd11_stat *sb);
__sym_compat(nlstat, freebsd11_nlstat, FBSD_1.0);
int freebsd11_nfstat(int fd, struct freebsd11_stat *sb);
__sym_compat(nfstat, freebsd11_nfstat, FBSD_1.0);

/* stat(2) */
static inline abi_long do_freebsd11_stat(abi_long arg1, abi_long arg2)
{
    abi_long ret;
    void *p;
    struct stat st;

    LOCK_PATH(p, arg1);
    ret = get_errno(stat(path(p), &st));
    UNLOCK_PATH(p, arg1);
    if (!is_error(ret)) {
        ret = h2t_freebsd11_stat(arg2, &st);
    }
    return ret;
}

/* lstat(2) */
static inline abi_long do_freebsd11_lstat(abi_long arg1, abi_long arg2)
{
    abi_long ret;
    void *p;
    struct stat st;

    LOCK_PATH(p, arg1);
    ret = get_errno(lstat(path(p), &st));
    UNLOCK_PATH(p, arg1);
    if (!is_error(ret)) {
        ret = h2t_freebsd11_stat(arg2, &st);
    }
    return ret;
}

/* fstat(2) */
static inline abi_long do_freebsd11_fstat(abi_long arg1, abi_long arg2)
{
    abi_long ret;
    struct stat st;

    ret = get_errno(fstat(arg1, &st));
    if (!is_error(ret))  {
	    ret = h2t_freebsd11_stat(arg2, &st);
    }
    return ret;
}

/* fstat(2) */
static inline abi_long do_freebsd_fstat(abi_long arg1, abi_long arg2)
{
    abi_long ret;
    struct stat st;

    ret = get_errno(fstat(arg1, &st));
    if (!is_error(ret))  {
        ret = h2t_freebsd_stat(arg2, &st);
    }
    return ret;
}

/* fstatat(2) */
static inline abi_long do_freebsd11_fstatat(abi_long arg1, abi_long arg2,
        abi_long arg3, abi_long arg4)
{
    abi_long ret;
    void *p;
    struct stat st;

    LOCK_PATH(p, arg2);
    if (arg1 == AT_FDCWD)
        arg1 = HOST_AT_FDCWD;
    abi_long hflags = target_to_host_bitmask(arg4, at_flags_tbl);
    ret = get_errno(fstatat(arg1, p, &st, hflags));
    UNLOCK_PATH(p, arg2);
    if (!is_error(ret) && arg3) {
        ret = h2t_freebsd11_stat(arg3, &st);
    }
    return ret;
}

/* fstatat(2) */
static inline abi_long do_freebsd_fstatat(abi_int arg1, abi_long arg2,
        abi_long arg3, abi_long arg4)
{
    abi_long ret;
    void *p;
    struct stat st;

    LOCK_PATH(p, arg2);
    if (arg1 == AT_FDCWD)
        arg1 = HOST_AT_FDCWD;
    abi_long hflags = target_to_host_bitmask(arg4, at_flags_tbl);
    ret = get_errno(fstatat(arg1, p, &st, hflags));
    UNLOCK_PATH(p, arg2);
    if (!is_error(ret) && arg3) {
        ret = h2t_freebsd_stat(arg3, &st);
    }
    return ret;
}

/* undocumented nstat(char *path, struct nstat *ub) syscall */
static abi_long do_freebsd11_nstat(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)
    abi_long ret;
    void *p;
    struct freebsd11_stat st;

    LOCK_PATH(p, arg1);
    ret = get_errno(freebsd11_nstat(path(p), &st));
    UNLOCK_PATH(p, arg1);
    if (!is_error(ret)) {
        ret = h2t_freebsd11_nstat(arg2, &st);
    }
    return ret;
#else
    abort();
#endif
}

/* undocumented nfstat(int fd, struct nstat *sb) syscall */
static abi_long do_freebsd11_nfstat(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)
    abi_long ret;
    struct freebsd11_stat st;

    ret = get_errno(freebsd11_nfstat(arg1, &st));
    if (!is_error(ret))  {
        ret = h2t_freebsd11_nstat(arg2, &st);
    }
    return ret;
#else
    abort();
#endif
}

/* undocumented nlstat(char *path, struct nstat *ub) syscall */
static abi_long do_freebsd11_nlstat(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)
    abi_long ret;
    void *p;
    struct freebsd11_stat st;

    LOCK_PATH(p, arg1);
    ret = get_errno(freebsd11_nlstat(path(p), &st));
    UNLOCK_PATH(p, arg1);
    if (!is_error(ret)) {
        ret = h2t_freebsd11_nstat(arg2, &st);
    }
    return ret;
#else
    abort();
#endif
}

/* getfh(2) */
static abi_long do_freebsd_getfh(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)
    abi_long ret;
    void *p;
    fhandle_t host_fh;

    LOCK_PATH(p, arg1);
    ret = get_errno(getfh(path(p), &host_fh));
    UNLOCK_PATH(p, arg1);
    if (is_error(ret)) {
        return ret;
    }
    return h2t_freebsd_fhandle(arg2, &host_fh);
#else
    abort();
#endif
}

/* lgetfh(2) */
static inline abi_long do_freebsd_lgetfh(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)
    abi_long ret;
    void *p;
    fhandle_t host_fh;

    LOCK_PATH(p, arg1);
    ret = get_errno(lgetfh(path(p), &host_fh));
    UNLOCK_PATH(p, arg1);
    if (is_error(ret)) {
        return ret;
    }
    return h2t_freebsd_fhandle(arg2, &host_fh);
#else
    abort();
#endif
}

/* fhopen(2) */
static inline abi_long do_freebsd_fhopen(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)
    abi_long ret;
    fhandle_t host_fh;

    ret = t2h_freebsd_fhandle(&host_fh, arg1);
    if (is_error(ret)) {
        return ret;
    }

    return get_errno(fhopen(&host_fh, arg2));
#else
    abort();
#endif
}

/* fhstat(2) */
static inline abi_long do_freebsd11_fhstat(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)
    abi_long ret;
    fhandle_t host_fh;
    struct freebsd11_stat host_sb;

    ret = t2h_freebsd_fhandle(&host_fh, arg1);
    if (is_error(ret)) {
        return ret;
    }
    ret = get_errno(freebsd11_fhstat(&host_fh, &host_sb));
    if (is_error(ret)) {
        return ret;
    }
    return h2t_freebsd11_stat(arg2, &host_sb);
#else
    abort();
#endif
}

/* fhstat(2) */
static inline abi_long do_freebsd_fhstat(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)
    abi_long ret;
    fhandle_t host_fh;
    struct stat host_sb;

    ret = t2h_freebsd_fhandle(&host_fh, arg1);
    if (is_error(ret)) {
        return ret;
    }
    ret = get_errno(fhstat(&host_fh, &host_sb));
    if (is_error(ret)) {
        return ret;
    }
    return h2t_freebsd_stat(arg2, &host_sb);
#else
    abort();
#endif
}

/* fhstatfs(2) */
static inline abi_long do_freebsd11_fhstatfs(abi_ulong target_fhp_addr,
        abi_ulong target_stfs_addr)
{
#if !defined(__linux__)
    abi_long ret;
    fhandle_t host_fh;
    struct freebsd11_statfs host_stfs;

    ret = t2h_freebsd_fhandle(&host_fh, target_fhp_addr);
    if (is_error(ret)) {
        return ret;
    }
    ret = get_errno(freebsd11_fhstatfs(&host_fh, &host_stfs));
    if (is_error(ret)) {
        return ret;
    }
    return h2t_freebsd11_statfs(target_stfs_addr, &host_stfs);
#else
    abort();
#endif
}

/* fhstatfs(2) */
static inline abi_long do_freebsd_fhstatfs(abi_ulong target_fhp_addr,
        abi_ulong target_stfs_addr)
{
#if !defined(__linux__)
    abi_long ret;
    fhandle_t host_fh;
    struct statfs host_stfs;

    ret = t2h_freebsd_fhandle(&host_fh, target_fhp_addr);
    if (is_error(ret)) {
        return ret;
    }
    ret = get_errno(fhstatfs(&host_fh, &host_stfs));
    if (is_error(ret)) {
        return ret;
    }
    return h2t_freebsd_statfs(target_stfs_addr, &host_stfs);
#else
    abort();
#endif
}

/* statfs(2) */
static inline abi_long do_freebsd11_statfs(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)
    abi_long ret;
    void *p;
    struct freebsd11_statfs host_stfs;

    LOCK_PATH(p, arg1);
    ret = get_errno(freebsd11_statfs(path(p), &host_stfs));
    UNLOCK_PATH(p, arg1);
    if (is_error(ret)) {
        return ret;
    }

    return h2t_freebsd11_statfs(arg2, &host_stfs);
#else
    abort();
#endif
}

/* statfs(2) */
static inline abi_long do_freebsd_statfs(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)
    abi_long ret;
    void *p;
    struct statfs host_stfs;

    LOCK_PATH(p, arg1);
    ret = get_errno(statfs(path(p), &host_stfs));
    UNLOCK_PATH(p, arg1);
    if (is_error(ret)) {
        return ret;
    }

    return h2t_freebsd_statfs(arg2, &host_stfs);
#else
    abort();
#endif
}

/* fstatfs(2) */
static inline abi_long do_freebsd11_fstatfs(abi_long fd, abi_ulong target_addr)
{
    abi_long ret;
    struct statfs host_stfs;

    ret = get_errno(fstatfs(fd, &host_stfs));
    if (is_error(ret)) {
        return ret;
    }

    return h2t_freebsd_statfs(target_addr, &host_stfs);
}

/* fstatfs(2) */
static inline abi_long do_freebsd_fstatfs(abi_long fd, abi_ulong target_addr)
{
#if !defined(__linux__)
    abi_long ret;
    struct statfs host_stfs;

    ret = get_errno(fstatfs(fd, &host_stfs));
    if (is_error(ret)) {
        return ret;
    }

    return h2t_freebsd_statfs(target_addr, &host_stfs);
#else
    abort();
#endif
}

/* getfsstat(2) */
static inline abi_long do_freebsd11_getfsstat(abi_ulong target_addr,
        abi_long bufsize, abi_long flags)
{
#if !defined(__linux__)
    abi_long ret;
    struct freebsd11_statfs *host_stfs;
    int count;
    long host_bufsize;

    count = bufsize / sizeof(struct target_freebsd11_statfs);

    /* if user buffer is NULL then return number of mounted FS's */
    if (target_addr == 0 || count == 0) {
        return get_errno(freebsd11_getfsstat(NULL, 0, flags));
    }

    /* XXX check count to be reasonable */
    host_bufsize = sizeof(struct freebsd11_statfs) * count;
    host_stfs = alloca(host_bufsize);
    if (!host_stfs) {
        return -TARGET_EINVAL;
    }

    ret = count = get_errno(freebsd11_getfsstat(host_stfs, host_bufsize, flags));
    if (is_error(ret)) {
        return ret;
    }

    while (count--) {
        if (h2t_freebsd11_statfs((target_addr +
                        (count * sizeof(struct target_freebsd11_statfs))),
                    &host_stfs[count])) {
            return -TARGET_EFAULT;
        }
    }
    return ret;
#else
    abort();
#endif
}

/* getfsstat(2) */
static inline abi_long do_freebsd_getfsstat(abi_ulong target_addr,
        abi_long bufsize, abi_long flags)
{
#if !defined(__linux__)
    abi_long ret;
    struct statfs *host_stfs;
    int count;
    long host_bufsize;

    count = bufsize / sizeof(struct target_statfs);

    /* if user buffer is NULL then return number of mounted FS's */
    if (target_addr == 0 || count == 0) {
        return get_errno(freebsd11_getfsstat(NULL, 0, flags));
    }

    /* XXX check count to be reasonable */
    host_bufsize = sizeof(struct statfs) * count;
    host_stfs = alloca(host_bufsize);
    if (!host_stfs) {
        return -TARGET_EINVAL;
    }

    ret = count = get_errno(getfsstat(host_stfs, host_bufsize, flags));
    if (is_error(ret)) {
        return ret;
    }

    while (count--) {
        if (h2t_freebsd_statfs((target_addr +
                        (count * sizeof(struct target_statfs))),
                    &host_stfs[count])) {
            return -TARGET_EFAULT;
        }
    }
    return ret;
#else
    abort();
#endif
}

/* getdents(2) */
static inline abi_long do_freebsd11_getdents(abi_long arg1,
        abi_ulong arg2, abi_long nbytes)
{
#if !defined(__linux__)
    abi_long ret;
    struct freebsd11_dirent *dirp;

    dirp = lock_user(VERIFY_WRITE, arg2, nbytes, 0);
    if (dirp == NULL) {
        return -TARGET_EFAULT;
    }
    ret = get_errno(freebsd11_getdents(arg1, (char *)dirp, nbytes));
    if (!is_error(ret)) {
        struct freebsd11_dirent *de;
        int len = ret;
        int reclen;

        de = dirp;
        while (len > 0) {
            reclen = de->d_reclen;
            if (reclen > len) {
                return -TARGET_EFAULT;
            }
            de->d_reclen = tswap16(reclen);
            de->d_fileno = tswap32(de->d_fileno);
            len -= reclen;
        }
    }
    return ret;
#else
    abort();
#endif
}

#define ALIGN(p, a) (((p) + ((a) - 1)) & ~((a) - 1))
#define aligned_alloca(n, a) ({void *_x = alloca(n + a - 1); (void *)ALIGN((uintptr_t)_x, (a));})

/* getdirentries(2) */
static inline abi_long do_freebsd11_getdirentries(abi_long arg1,
        abi_ulong arg2, abi_long nbytes, abi_ulong arg4)
{
    abi_long ret;
    struct freebsd11_dirent *dirp;
    struct dirent *adirp;
    off_t basep;

    dirp = lock_user(VERIFY_WRITE, arg2, nbytes, 0);
    if (dirp == NULL) {
        return -TARGET_EFAULT;
    }
    int h_nbytes = (nbytes * offsetof(struct dirent, d_name)) / offsetof(struct freebsd11_dirent, d_name);
    adirp = aligned_alloca(h_nbytes, __alignof__(*adirp));
    if (adirp == NULL) {
        ret = -TARGET_ENOMEM;
        goto out;
    }
    ret = get_errno(getdirentries(arg1, (char *)adirp, h_nbytes, &basep));
    if (ret <= 0)
        goto out;
    struct freebsd11_dirent *tde = dirp;
    struct dirent *hde = adirp;
    int len = ret;
    ret = 0;
    int reclen;
    off_t last_off = -1;

    while (len > 0) {
        reclen = hde->d_reclen;
        int namelen = reclen - offsetof(typeof(*hde), d_name);
        if (reclen > len || namelen <= 0) {
            abort();
        }
        int target_reclen = ALIGN(offsetof(typeof(*tde), d_name) + namelen, __alignof__(typeof(*tde)));
        if ((nbytes - ret) < (target_reclen * 8)) {
            assert(last_off > 0 && lseek(arg1, last_off, SEEK_SET) == last_off);
            basep = last_off;
            break;
        }
        *tde = (typeof(*tde)){0};
        abi_long tft = host_to_target_bitmask(DTTOIF(hde->d_type), fcntl_flags_tbl);
        tde->d_type = IFTODT(tft);
        last_off = hde->d_off;
        tde->d_fileno = tswap32(hde->d_fileno);
        tde->d_reclen = tswap16(target_reclen);
        char *ep = memchr(hde->d_name, '\0', namelen);
        if (ep > hde->d_name) {
            int sl = ep - hde->d_name;
            memcpy(tde->d_name, hde->d_name, sl + 1);
            tde->d_namlen = tswap16(sl);
        } else {
            abort();
        }
        len -= reclen;
        ret += target_reclen;
        tde = (typeof(tde))((void *)tde + target_reclen);
        hde = (typeof(hde))((void *)hde + reclen);
    }
    unlock_user(dirp, arg2, ret);
    if (arg4) {
        if (put_user(basep, arg4, abi_ulong)) {
            return -TARGET_EFAULT;
        }
    }
    return ret;
out:
    unlock_user(dirp, arg2, ret);
    return ret;
}

struct target_dirent {
        ino_t      d_fileno;            /* file number of entry */
        off_t      d_off;               /* directory offset of next entry */
        __uint16_t d_reclen;            /* length of this record */
        __uint8_t  d_type;              /* file type, see below */
        __uint8_t  d_pad0;
        __uint16_t d_namlen;            /* length of string in d_name */
        __uint16_t d_pad1;
        char    d_name[TARGET_MAXNAMLEN + 1];  /* name must be no longer than this */
};

/* getdirentries(2) */
static inline abi_long do_freebsd_getdirentries(abi_long arg1,
        abi_ulong arg2, abi_long nbytes, abi_ulong arg4)
{
    abi_long ret;
    struct target_dirent *dirp;
    struct dirent *adirp;
    off_t basep;

    dirp = lock_user(VERIFY_WRITE, arg2, nbytes, 0);
    if (dirp == NULL) {
        return -TARGET_EFAULT;
    }
    int h_nbytes = (nbytes * offsetof(struct dirent, d_name)) /
                             offsetof(struct target_dirent, d_name);
    adirp = aligned_alloca(h_nbytes, __alignof__(*adirp));
    if (adirp == NULL) {
        ret = -TARGET_ENOMEM;
        goto out;
    }
    ret = get_errno(getdirentries(arg1, (char *)adirp, h_nbytes, &basep));
    if (ret <= 0)
        goto out;
    struct target_dirent *tde = dirp;
    struct dirent *hde = adirp;
    int len = ret;
    ret = 0;
    int reclen;
    off_t last_off = -1;

    while (len > 0) {
        reclen = hde->d_reclen;
	int namelen = reclen - offsetof(typeof(*hde), d_name);
        if (reclen > len || namelen <= 0) {
            abort();
        }
        int target_reclen = ALIGN(offsetof(typeof(*tde), d_name) + namelen, __alignof__(typeof(*tde)));
        if ((nbytes - ret) < (target_reclen * 8)) {
            assert(last_off > 0 && lseek(arg1, last_off, SEEK_SET) == last_off);
            basep = last_off;
            goto early_out;
        }
        *tde = (typeof(*tde)){0};
        abi_long tft = host_to_target_bitmask(DTTOIF(hde->d_type), fcntl_flags_tbl);
        tde->d_type = IFTODT(tft);
        last_off = tde->d_off = hde->d_off;
        tde->d_fileno = tswap32(hde->d_fileno);
        tde->d_reclen = tswap16(target_reclen);
        char *ep = memchr(hde->d_name, '\0', namelen);
        if (ep > hde->d_name) {
            int sl = ep - hde->d_name;
            memcpy(tde->d_name, hde->d_name, sl + 1);
	    assert(tde->d_name[sl] == '\0');
            tde->d_namlen = tswap16(sl);
        } else {
            abort();
        }
        len -= reclen;
        ret += target_reclen;
        tde = (typeof(tde))((void *)tde + target_reclen);
        hde = (typeof(hde))((void *)hde + reclen);
    }
    assert(len == 0);
early_out:
    unlock_user(dirp, arg2, ret);
    if (arg4) {
        if (put_user(basep, arg4, abi_ulong)) {
            return -TARGET_EFAULT;
        }
    }
    assert(ret <= nbytes);
    return ret;
out:
    unlock_user(dirp, arg2, ret);
    return ret;
}

/* fcntl(2) */
static inline abi_long do_freebsd_fcntl(abi_long arg1, abi_long arg2,
        abi_ulong arg3)
{
    abi_long ret;
    int host_cmd;
    struct flock fl;
    struct target_freebsd_flock *target_fl;
    const abi_long STUB = 0;

    host_cmd = target_to_host_fcntl_cmd(arg2);
    if (host_cmd < 0) {
        return host_cmd;
    }
    switch (arg2) {
    case TARGET_F_GETLK:
        if (!lock_user_struct(VERIFY_READ, target_fl, arg3, 1)) {
            return -TARGET_EFAULT;
        }
        __get_user(fl.l_type, &target_fl->l_type);
        __get_user(fl.l_whence, &target_fl->l_whence);
        __get_user(fl.l_start, &target_fl->l_start);
        __get_user(fl.l_len, &target_fl->l_len);
        __get_user(fl.l_pid, &target_fl->l_pid);
        unlock_user_struct(target_fl, arg3, 0);
        ret = get_errno(safe_fcntl(arg1, host_cmd, &fl));
        if (!is_error(ret)) {
            if (!lock_user_struct(VERIFY_WRITE, target_fl, arg3, 0)) {
                return -TARGET_EFAULT;
            }
            __put_user(fl.l_type, &target_fl->l_type);
            __put_user(fl.l_whence, &target_fl->l_whence);
            __put_user(fl.l_start, &target_fl->l_start);
            __put_user(fl.l_len, &target_fl->l_len);
            __put_user(fl.l_pid, &target_fl->l_pid);
            __put_user(STUB, &target_fl->l_sysid);
            unlock_user_struct(target_fl, arg3, 1);
        }
        break;

    case TARGET_F_SETLK:
    case TARGET_F_SETLKW:
        if (!lock_user_struct(VERIFY_READ, target_fl, arg3, 1)) {
            return -TARGET_EFAULT;
        }
        __get_user(fl.l_type, &target_fl->l_type);
        __get_user(fl.l_whence, &target_fl->l_whence);
        __get_user(fl.l_start, &target_fl->l_start);
        __get_user(fl.l_len, &target_fl->l_len);
        __get_user(fl.l_pid, &target_fl->l_pid);
        unlock_user_struct(target_fl, arg3, 0);
        ret = get_errno(safe_fcntl(arg1, host_cmd, &fl));
        break;

    case TARGET_F_DUPFD:
    case TARGET_F_DUP2FD:
    case TARGET_F_GETOWN:
    case TARGET_F_SETOWN:
    case TARGET_F_GETFD:
    case TARGET_F_SETFD:
    case TARGET_F_GETFL:
    case TARGET_F_SETFL:
    case TARGET_F_READAHEAD:
    case TARGET_F_RDAHEAD:
    case TARGET_F_ADD_SEALS:
    case TARGET_F_GET_SEALS:
    default:
        ret = get_errno(safe_fcntl(arg1, host_cmd, arg3));
        break;
    }
    return ret;
}

#if defined(__FreeBSD_version) && __FreeBSD_version >= 1300080
extern int __realpathat(int fd, const char *path, char *buf, size_t size,
        int flags);
/* https://svnweb.freebsd.org/base?view=revision&revision=358172 */
/* no man page */
static inline abi_long do_freebsd_realpathat(abi_int arg1, abi_long arg2,
        abi_long arg3, abi_long arg4, abi_int arg5)
{
    abi_long ret;
    void *p, *b;

    LOCK_PATH(p, arg2);
    b = lock_user(VERIFY_WRITE, arg3, arg4, 0);
    if (b == NULL) {
        UNLOCK_PATH(p, arg2);
        return -TARGET_EFAULT;
    }

    if (arg1 != AT_FDCWD || arg5 != 0)
        abort();
    char *rets = realpath(p, NULL);
    if (rets == NULL) {
        ret = get_errno(-1);
        goto out;
    }
    int plen = strlen(rets) + 1;
    if (arg4 < plen) {
        free(rets);
        ret = -TARGET_ENOMEM;
        goto out;
    }
    memcpy(b, rets, plen);
    free(rets);
    ret = 0;
 out:
    UNLOCK_PATH(p, arg2);
    unlock_user(b, arg3, ret);

    return ret;
}
#endif

#endif /* BSD_USER_FREEBSD_OS_STAT_H */
