/*
 *  miscellaneous FreeBSD system call shims
 *
 *  Copyright (c) 2013-14 Stacey D. Son
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

#ifndef OS_MISC_H
#define OS_MISC_H

#include <sys/random.h>
#include <sched.h>

int shm_open2(const char *path, int flags, mode_t mode, int shmflags,
    const char *);

/* sched_setparam(2) */
static inline abi_long do_freebsd_sched_setparam(pid_t pid,
        abi_ulong target_sp_addr)
{
    abi_long ret;
    struct sched_param host_sp;

    ret = get_user_s32(host_sp.sched_priority, target_sp_addr);
    if (!is_error(ret)) {
        ret = get_errno(sched_setparam(pid, &host_sp));
    }
    return ret;
}

/* sched_get_param(2) */
static inline abi_long do_freebsd_sched_getparam(pid_t pid,
        abi_ulong target_sp_addr)
{
    abi_long ret;
    struct sched_param host_sp;

    ret = get_errno(sched_getparam(pid, &host_sp));
    if (!is_error(ret)) {
        ret = put_user_s32(host_sp.sched_priority, target_sp_addr);
    }
    return ret;
}

/* sched_setscheduler(2) */
static inline abi_long do_freebsd_sched_setscheduler(pid_t pid, int policy,
        abi_ulong target_sp_addr)
{
    abi_long ret;
    struct sched_param host_sp;

    ret = get_user_s32(host_sp.sched_priority, target_sp_addr);
    if (!is_error(ret)) {
        ret = get_errno(sched_setscheduler(pid, policy, &host_sp));
    }
    return ret;
}

/* sched_getscheduler(2) */
static inline abi_long do_freebsd_sched_getscheduler(pid_t pid)
{

    return get_errno(sched_getscheduler(pid));
}

/* sched_getscheduler(2) */
static inline abi_long do_freebsd_sched_rr_get_interval(pid_t pid,
        abi_ulong target_ts_addr)
{
    abi_long ret;
    struct timespec host_ts;

    ret = get_errno(sched_rr_get_interval(pid, &host_ts));
    if (!is_error(ret)) {
        ret = h2t_freebsd_timespec(target_ts_addr, &host_ts);
    }
    return ret;
}

/* cpuset(2) */
static inline abi_long do_freebsd_cpuset(abi_ulong target_cpuid)
{
#if !defined(__linux__)
    abi_long ret;
    cpusetid_t setid;

    ret = get_errno(cpuset(&setid));
    if (is_error(ret)) {
        return ret;
    }
    return put_user_s32(setid, target_cpuid);
#else
    abort();
#endif
}

#define target_to_host_cpuset_which(hp, t) { \
    (*hp) = t;                               \
} while (0)

#define target_to_host_cpuset_level(hp, t) { \
    (*hp) = t;                               \
} while (0)

/* cpuset_setid(2) */
static inline abi_long do_freebsd_cpuset_setid(void *cpu_env, abi_long arg1,
        abi_ulong arg2, abi_ulong arg3, abi_ulong arg4, abi_ulong arg5)
{
#if !defined(__linux__)
    id_t id;    /* 64-bit value */
    cpusetid_t setid;
    cpuwhich_t which;

    target_to_host_cpuset_which(&which, arg1);
#if TARGET_ABI_BITS == 32
    /* See if we need to align the register pairs */
    if (regpairs_aligned(cpu_env)) {
        id = target_arg64(arg3, arg4);
        setid = arg5;
    } else {
        id = target_arg64(arg2, arg3);
        setid = arg4;
    }
#else
    id = arg2;
    setid = arg3;
#endif
    return get_errno(cpuset_setid(which, id, setid));
#else
    abort();
#endif
}

/* cpuset_getid(2) */
static inline abi_long do_freebsd_cpuset_getid(abi_long arg1, abi_ulong arg2,
        abi_ulong arg3, abi_ulong arg4, abi_ulong arg5)
{
#if !defined(__linux__)
    abi_long ret;
    id_t id;    /* 64-bit value */
    cpusetid_t setid;
    cpuwhich_t which;
    cpulevel_t level;
    abi_ulong target_setid;

    target_to_host_cpuset_which(&which, arg1)
	;
    target_to_host_cpuset_level(&level, arg2);
#if TARGET_ABI_BITS == 32
    id = target_arg64(arg3, arg4);
    target_setid = arg5;
#else
    id = arg3;
    target_setid = arg4;
#endif
    ret = get_errno(cpuset_getid(level, which, id, &setid));
    if (is_error(ret)) {
        return ret;
    }
    return put_user_s32(setid, target_setid);
#else
    abort();
#endif
}

#if defined(__linux__)
typedef void * cpuset_t;
typedef abi_long cpulevel_t;
typedef abi_long cpuwhich_t;
#endif

#if !defined(__linux__)
static abi_ulong copy_from_user_cpuset_mask(cpuset_t *mask,
	abi_ulong target_mask_addr)
{
	int i, j, k;
	abi_ulong b, *target_mask;

	target_mask = lock_user(VERIFY_READ, target_mask_addr,
					(CPU_SETSIZE / 8), 1);
	if (target_mask == NULL) {
		return -TARGET_EFAULT;
	}
	CPU_ZERO(mask);
	k = 0;
	for (i = 0; i < ((CPU_SETSIZE/8)/sizeof(abi_ulong)); i++) {
		__get_user(b, &target_mask[i]);
		for (j = 0; j < TARGET_ABI_BITS; j++) {
			if ((b >> j) & 1) {
				CPU_SET(k, mask);
			}
			k++;
		}
	}
	unlock_user(target_mask, target_mask_addr, 0);

	return 0;
}

static abi_ulong copy_to_user_cpuset_mask(abi_ulong target_mask_addr,
	cpuset_t *mask)
{
	int i, j, k;
	abi_ulong b, *target_mask;

	target_mask = lock_user(VERIFY_WRITE, target_mask_addr,
					(CPU_SETSIZE / 8), 0);
	if (target_mask == NULL) {
		return -TARGET_EFAULT;
	}
	k = 0;
	for (i = 0; i < ((CPU_SETSIZE/8)/sizeof(abi_ulong)); i++) {
		b = 0;
		for (j = 0; j < TARGET_ABI_BITS; j++) {
			b |= ((CPU_ISSET(k, mask) != 0) << j);
			k++;
		}
		__put_user(b, &target_mask[i]);
	}
	unlock_user(target_mask, target_mask_addr, (CPU_SETSIZE / 8));

	return 0;
}
#endif

/* cpuset_getaffinity(2) */
/* cpuset_getaffinity(cpulevel_t, cpuwhich_t, id_t, size_t, cpuset_t *); */
static inline abi_long do_freebsd_cpuset_getaffinity(cpulevel_t level,
        cpuwhich_t which, abi_ulong arg3, abi_ulong arg4, abi_ulong arg5,
        abi_ulong arg6)
{
#if !defined(__linux__)
	cpuset_t mask;
	abi_long ret;
    id_t id;    /* 64-bit */
    abi_ulong setsize, target_mask;

#if TARGET_ABI_BITS == 32
    id = (id_t)target_arg64(arg3, arg4);
    setsize = arg5;
    target_mask = arg6;
#else
    id = (id_t)arg3;
    setsize = arg4;
    target_mask = arg5;
#endif

	ret = get_errno(cpuset_getaffinity(level, which, id, setsize, &mask));
	if (ret == 0) {
		ret = copy_to_user_cpuset_mask(target_mask, &mask);
	}

    return ret;
#else
    abort();
#endif
}

/* cpuset_setaffinity(2) */
/* cpuset_setaffinity(cpulevel_t, cpuwhich_t, id_t, size_t, const cpuset_t *);*/
static inline abi_long do_freebsd_cpuset_setaffinity(cpulevel_t level,
        cpuwhich_t which, abi_ulong arg3, abi_ulong arg4, abi_ulong arg5,
        abi_ulong arg6)
{
#if !defined(__linux__)
	cpuset_t mask;
	abi_long ret;
    id_t id; /* 64-bit */
    abi_ulong setsize, target_mask;

#if TARGET_ABI_BITS == 32
    id = (id_t)target_arg64(arg3, arg4);
    setsize = arg5;
    target_mask = arg6;
#else
    id = (id_t)arg3;
    setsize = arg4;
    target_mask = arg5;
#endif

	ret = copy_from_user_cpuset_mask(&mask, target_mask);
	if (ret == 0) {
		ret = get_errno(cpuset_setaffinity(level, which, id, setsize, &mask));
	}

	return ret;
#else
    abort();
#endif
}

/*
 * Pretend there are no modules loaded into the kernel. Don't allow loading or
 * unloading of modules. This works well for tests, and little else seems to
 * care. Will reevaluate if examples are found that do matter.
 */

/* modfnext(2) */
static inline abi_long do_freebsd_modfnext(abi_long modid)
{
    return -TARGET_ENOENT;
}

/* modfind(2) */
static inline abi_long do_freebsd_modfind(abi_ulong target_name)
{
    return -TARGET_ENOENT;
}

/* kldload(2) */
static inline abi_long do_freebsd_kldload(abi_ulong target_name)
{
    return -TARGET_EPERM;        /* You can't load kernel modules is the best error */
}

/* kldunload(2) */
static inline abi_long do_freebsd_kldunload(abi_long fileid)
{
    return -TARGET_EPERM;        /* You can't unload kernel modules is the best error */
}

/* kldunloadf(2) */
static inline abi_long do_freebsd_kldunloadf(abi_long fileid, abi_long flags)
{
    return -TARGET_EPERM;        /* You can't unload kernel modules is the best error */
}

/* kldfind(2) */
static inline abi_long do_freebsd_kldfind(abi_ulong target_name)
{
    return -TARGET_ENOENT;
}

/* kldnext(2) */
static inline abi_long do_freebsd_kldnext(abi_long fileid)
{
    return -TARGET_ENOENT;
}


/* kldstat(2) */
static inline abi_long do_freebsd_kldstat(abi_long fileid,
        abi_ulong target_stat)
{
    return -TARGET_ENOENT;
}

/* kldfirstmod(2) */
static inline abi_long do_freebsd_kldfirstmod(abi_long fileid)
{
    return -TARGET_ENOENT;
}

/* kldsym(2) */
static inline abi_long do_freebsd_kldsym(abi_long fileid, abi_long cmd,
        abi_ulong target_data)
{
    return -TARGET_ENOENT;
}

/*
 * New posix calls
 */

#if TARGET_ABI_BITS == 32
static inline uint64_t target_offset64(uint32_t word0, uint32_t word1)
{
#ifdef TARGET_BIG_ENDIAN
    return ((uint64_t)word0 << 32) | word1;
#else
    return ((uint64_t)word1 << 32) | word0;
#endif
}
#else /* TARGET_ABI_BITS == 32 */
static inline uint64_t target_offset64(uint64_t word0, uint64_t word1)
{
    return word0;
}
#endif /* TARGET_ABI_BITS != 32 */

/* posix_fallocate(2) */
static inline abi_long do_freebsd_posix_fallocate(abi_long arg1, abi_long arg2, abi_long arg3, abi_long arg4, abi_long arg5, abi_long arg6)
{

#if TARGET_ABI_BITS == 32                           
    return get_errno(posix_fallocate(arg1, target_offset64(arg3, arg4),
        target_offset64(arg5, arg6)));
#else
    return get_errno(posix_fallocate(arg1, arg2, arg3));
#endif
}

/* posix_openpt(2) */
static inline abi_long do_freebsd_posix_openpt(abi_long flags)
{

    return get_errno(posix_openpt(flags));
}

/*
 * shm_open2 isn't exported, but the __sys_ alias is. We can use either for the
 * static version, but to dynamically link we have to use the sys version.
 */
int __sys_shm_open2(const char *path, int flags, mode_t mode, int shmflags,
    const char *);

#if defined(__FreeBSD_version) && __FreeBSD_version >= 1300048
/* shm_open2(2) */
static inline abi_long do_freebsd_shm_open2(abi_ulong pathptr, abi_ulong flags,
    abi_long mode, abi_ulong shmflags, abi_ulong nameptr)
{
    int ret;
    void *uname, *upath;

    if (pathptr == (uintptr_t)SHM_ANON) {
        upath = SHM_ANON;
    } else {
        upath = lock_user_string(pathptr);
        if (upath == NULL) {
            return -TARGET_EFAULT;
        }
    }

    uname = NULL;
    if (nameptr != 0) {
        uname = lock_user_string(nameptr);
        if (uname == NULL) {
            unlock_user(upath, pathptr, 0);
            return -TARGET_EFAULT;
        }
    }
#if !defined(__linux__)
    ret = get_errno(__sys_shm_open2(upath,
                target_to_host_bitmask(flags, fcntl_flags_tbl), mode,
                target_to_host_bitmask(shmflags, shmflag_flags_tbl), uname));
#else
    abort();
#endif

    if (upath != SHM_ANON) {
        unlock_user(upath, pathptr, 0);
    }
    if (uname != NULL) {
        unlock_user(uname, nameptr, 0);
    }
    return ret;
}
#endif /* __FreeBSD_version >= 1300048 */

#if defined(__FreeBSD_version) && __FreeBSD_version >= 1300049
/* shm_rename(2) */
static inline abi_long do_freebsd_shm_rename(abi_ulong fromptr, abi_ulong toptr,
        abi_ulong flags)
{
#if !defined(__linux__)
    int ret;
    void *ufrom, *uto;

    ufrom = lock_user_string(fromptr);
    if (ufrom == NULL) {
        return -TARGET_EFAULT;
    }
    uto = lock_user_string(toptr);
    if (uto == NULL) {
        unlock_user(ufrom, fromptr, 0);
        return -TARGET_EFAULT;
    }
    ret = get_errno(shm_rename(ufrom, uto, flags));
    unlock_user(ufrom, fromptr, 0);
    unlock_user(uto, toptr, 0);

    return ret;
#else
    abort();
#endif
}
#endif /* __FreeBSD_version >= 1300049 */

#if defined(CONFIG_GETRANDOM)
static inline abi_long do_freebsd_getrandom(abi_ulong buf, abi_ulong buflen,
        abi_ulong flags)
{
    abi_long ret;
    void *p;

    p = lock_user(VERIFY_WRITE, buf, buflen, 0);
    if (p == NULL) {
        return -TARGET_EFAULT;
    }
    ret = get_errno(getrandom(p, buflen, flags));
    unlock_user(p, buf, ret);

    return ret;
}
#endif

static inline abi_long do_freebsd_kenv(abi_long action, abi_ulong name,
    abi_ulong value, abi_long len)
{
#if !defined(__linux__)
    abi_long ret;
    void *gname = NULL;		/* unlocked in cases where set */
    void *gvalue = NULL;        /* unlocked in cases where set */

    ret = -TARGET_EINVAL;
    switch (action) {
    case KENV_GET:
        gname = lock_user_string(name);
	if (gname == NULL) {
	    ret = -TARGET_EFAULT;
	    break;
	}
	gvalue = lock_user(VERIFY_WRITE, value, len, 0);
	if (gvalue == NULL) {
	    ret = -TARGET_EFAULT;
	    break;
	}
	ret = kenv(action, gname, gvalue, len);
	if (ret > 0) {
		len = ret;
	}
	break;
    case KENV_SET:
        gname = lock_user_string(name);
	if (gname == NULL) {
	    ret = -TARGET_EFAULT;
	    break;
	}
	gvalue = lock_user(VERIFY_READ, value, len, 1);
	if (gvalue == NULL) {
	    ret = -TARGET_EFAULT;
	    break;
	}
	ret = kenv(action, gname, gvalue, len);
	break;
    case KENV_UNSET:
        gname = lock_user_string(name);
	if (gname == NULL) {
	    ret = -TARGET_EFAULT;
	    break;
	}
	ret = kenv(action, gname, NULL, 0);	/* value and name ignored, per kenv(2) */
	break;
    case KENV_DUMP:		/* All three treated the same */
    case KENV_DUMP_LOADER:
    case KENV_DUMP_STATIC:
	if (value != 0) {			/* value == NULL -> just return length */
	    gvalue = lock_user(VERIFY_WRITE, value, len, 0);
	    if (gvalue == NULL) {
		ret = -TARGET_EFAULT;
		break;
	    }
	}
	ret = kenv(action, NULL, gvalue, len);	/* name is ignored, per kenv(2) */
	if (ret > 0) {
		len = ret;
	}
	break;
    default:
	ret = -TARGET_EINVAL;
	break;
    }

    /* Unmap everything mapped */
    if (gvalue != NULL) {
	unlock_user(gvalue, value, len);
    }
    if (gname != NULL) {
	unlock_user(gname, name, 0);
    }

    return ret;
#else
    abort();
#endif
}

#endif /* OS_MISC_H */
