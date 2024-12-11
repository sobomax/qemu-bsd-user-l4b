/*
 *  FreeBSD filio definitions for ioctl(2) emulation
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

#ifndef BSD_USER_FREEBSD_OS_IOCTL_FILIO_H
#define BSD_USER_FREEBSD_OS_IOCTL_FILIO_H

/* see sys/filio.h */
#define TARGET_FIOCLEX      TARGET_IO('f', 1)
#define TARGET_FIONCLEX     TARGET_IO('f', 2)
#define TARGET_FIONREAD     TARGET_IOR('f', 127, int)
#define TARGET_FIONBIO      TARGET_IOW('f', 126, int)
#define TARGET_FIOASYNC     TARGET_IOW('f', 125, int)
#define TARGET_FIOSETOWN    TARGET_IOW('f', 124, int)
#define TARGET_FIOGETOWN    TARGET_IOR('f', 123, int)
#define TARGET_FIODTYPE     TARGET_IOR('f', 122, int)
#define TARGET_FIOGETLBA    TARGET_IOR('f', 121, int)

struct target_fiodgname_arg {
    int32_t     len;
    abi_ulong   buf;
};

#define TARGET_FIODGNAME    TARGET_IOW('f', 120,    \
                struct target_fiodgname_arg)
#define TARGET_FIONWRITE    TARGET_IOR('f', 119, int)
#define TARGET_FIONSPACE    TARGET_IOR('f', 118, int)
#define TARGET_FIOSEEKDATA  TARGET_IOWR('f', 97, off_t)
#define TARGET_FIOSEEKHOLE  TARGET_IOWR('f', 98, off_t)

#if defined(__linux__)
static abi_long do_ioctl_FIODTYPE(__unused const IOCTLEntry *ie,
                                  __unused uint8_t *buf_temp,
                                  int fd, __unused abi_long cmd,
                                  abi_long arg)
{
        abi_int *argptr;
        int target_size = sizeof(abi_int);
        struct stat st;
        int res = 0;

        argptr = lock_user(VERIFY_WRITE, arg, target_size, 0);
        if (!argptr) {
                return -TARGET_EFAULT;
        }
        if (fstat(fd, &st) != 0) {
                return -TARGET_EBADF;
        }
        if (S_ISCHR(st.st_mode) && isatty(fd)) {
                res = TARGET_D_TTY;
        } else if (S_ISBLK(st.st_mode)) {
                res = TARGET_D_DISK;
        }
        *argptr = res;
        unlock_user(argptr, arg, target_size);
        return 0;
}
#endif

#endif /* BSD_USER_FREEBSD_OS_IOCTL_FILIO_H */
