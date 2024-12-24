/*
 *  ioctl system call definitions
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

#ifndef BSD_IOCTL_H
#define BSD_IOCTL_H

abi_long do_bsd_ioctl(int fd, abi_long cmd, abi_long arg);
void init_bsd_ioctl(void);

static const bitmask_transtbl iflag_tbl[] = {
    { TARGET_IGNBRK,  TARGET_IGNBRK,  IGNBRK,  IGNBRK },
    { TARGET_BRKINT,  TARGET_BRKINT,  BRKINT,  BRKINT },
    { TARGET_IGNPAR,  TARGET_IGNPAR,  IGNPAR,  IGNPAR },
    { TARGET_PARMRK,  TARGET_PARMRK,  PARMRK,  PARMRK },
    { TARGET_INPCK,   TARGET_INPCK,   INPCK,   INPCK },
    { TARGET_ISTRIP,  TARGET_ISTRIP,  ISTRIP,  ISTRIP },
    { TARGET_INLCR,   TARGET_INLCR,   INLCR,   INLCR },
    { TARGET_IGNCR,   TARGET_IGNCR,   IGNCR,   IGNCR },
    { TARGET_ICRNL,   TARGET_ICRNL,   ICRNL,   ICRNL },
    { TARGET_IXON,    TARGET_IXON,    IXON,    IXON },
    { TARGET_IXANY,   TARGET_IXANY,   IXANY,   IXANY },
    { TARGET_IXOFF,   TARGET_IXOFF,   IXOFF,   IXOFF },
    { TARGET_IMAXBEL, TARGET_IMAXBEL, IMAXBEL, IMAXBEL },
    { TARGET_IUTF8,   TARGET_IUTF8,   IUTF8,   IUTF8 },
};

#define TARGET_CRDLY (TARGET_CR1 | TARGET_CR2 | TARGET_CR3)

static const bitmask_transtbl oflag_tbl[] = {
    { TARGET_OPOST,  TARGET_OPOST,  OPOST,  OPOST },
    { TARGET_ONLCR,  TARGET_ONLCR,  ONLCR,  ONLCR },
    { TARGET_OCRNL,  TARGET_OCRNL,  OCRNL,  OCRNL },
    { TARGET_ONOCR,  TARGET_ONOCR,  ONOCR,  ONOCR },
    { TARGET_ONLRET, TARGET_ONLRET, ONLRET, ONLRET },
    { TARGET_NL1,    TARGET_NL1,    NLDLY,  NL1 },
    { TARGET_CRDLY,  TARGET_CR1,    CRDLY,  CR1 },
    { TARGET_CRDLY,  TARGET_CR2,    CRDLY,  CR2 },
    { TARGET_CRDLY,  TARGET_CR3,    CRDLY,  CR3 },
    { TARGET_TABDLY, TARGET_TAB3,   TABDLY, TAB3 },
    { TARGET_BS1,    TARGET_BS1,    BSDLY,  BS1 },
    { TARGET_FF1,    TARGET_FF1,    FFDLY,  FF1 },
};

#define TARGET_CBAUD (TARGET_B50 | TARGET_B75 | TARGET_B110 | \
  TARGET_B134 | TARGET_B150 | TARGET_B200 | TARGET_B300 | \
  TARGET_B600 | TARGET_B1200 | TARGET_B1800 | TARGET_B2400 | \
  TARGET_B4800 | TARGET_B9600 | TARGET_B19200 | TARGET_B38400 | \
  TARGET_B57600 | TARGET_B115200 | TARGET_B230400 | TARGET_B460800)

static const bitmask_transtbl cflag_tbl[] = {
    { TARGET_CBAUD,   TARGET_B50,     CBAUD,   B50 },
    { TARGET_CBAUD,   TARGET_B75,     CBAUD,   B75 },
    { TARGET_CBAUD,   TARGET_B110,    CBAUD,   B110 },
    { TARGET_CBAUD,   TARGET_B134,    CBAUD,   B134 },
    { TARGET_CBAUD,   TARGET_B150,    CBAUD,   B150 },
    { TARGET_CBAUD,   TARGET_B200,    CBAUD,   B200 },
    { TARGET_CBAUD,   TARGET_B300,    CBAUD,   B300 },
    { TARGET_CBAUD,   TARGET_B600,    CBAUD,   B600 },
    { TARGET_CBAUD,   TARGET_B1200,   CBAUD,   B1200 },
    { TARGET_CBAUD,   TARGET_B1800,   CBAUD,   B1800 },
    { TARGET_CBAUD,   TARGET_B2400,   CBAUD,   B2400 },
    { TARGET_CBAUD,   TARGET_B4800,   CBAUD,   B4800 },
    { TARGET_CBAUD,   TARGET_B9600,   CBAUD,   B9600 },
    { TARGET_CBAUD,   TARGET_B19200,  CBAUD,   B19200 },
    { TARGET_CBAUD,   TARGET_B38400,  CBAUD,   B38400 },
    { TARGET_CBAUD,   TARGET_B57600,  CBAUD,   B57600 },
    { TARGET_CBAUD,   TARGET_B115200, CBAUD,   B115200 },
    { TARGET_CBAUD,   TARGET_B230400, CBAUD,   B230400 },
    { TARGET_CBAUD,   TARGET_B460800, CBAUD,   B460800 },
    { TARGET_CSIZE,   TARGET_CS6,     CSIZE,   CS6 },
    { TARGET_CSIZE,   TARGET_CS7,     CSIZE,   CS7 },
    { TARGET_CSIZE,   TARGET_CS8,     CSIZE,   CS8 },
    { TARGET_CSTOPB,  TARGET_CSTOPB,  CSTOPB,  CSTOPB },
    { TARGET_CREAD,   TARGET_CREAD,   CREAD,   CREAD },
    { TARGET_PARENB,  TARGET_PARENB,  PARENB,  PARENB },
    { TARGET_PARODD,  TARGET_PARODD,  PARODD,  PARODD },
    { TARGET_HUPCL,   TARGET_HUPCL,   HUPCL,   HUPCL },
    { TARGET_CLOCAL,  TARGET_CLOCAL,  CLOCAL,  CLOCAL },
    { TARGET_CRTSCTS, TARGET_CRTSCTS, CRTSCTS, CRTSCTS },
};

static const bitmask_transtbl lflag_tbl[] = {
    { TARGET_ISIG,    TARGET_ISIG,    ISIG,    ISIG },
    { TARGET_ICANON,  TARGET_ICANON,  ICANON,  ICANON },
    { TARGET_ECHO,    TARGET_ECHO,    ECHO,    ECHO },
    { TARGET_ECHOE,   TARGET_ECHOE,   ECHOE,   ECHOE },
    { TARGET_ECHOK,   TARGET_ECHOK,   ECHOK,   ECHOK },
    { TARGET_ECHONL,  TARGET_ECHONL,  ECHONL,  ECHONL },
    { TARGET_NOFLSH,  TARGET_NOFLSH,  NOFLSH,  NOFLSH },
    { TARGET_TOSTOP,  TARGET_TOSTOP,  TOSTOP,  TOSTOP },
    { TARGET_ECHOCTL, TARGET_ECHOCTL, ECHOCTL, ECHOCTL },
    { TARGET_ECHOPRT, TARGET_ECHOPRT, ECHOPRT, ECHOPRT },
    { TARGET_ECHOKE,  TARGET_ECHOKE,  ECHOKE,  ECHOKE },
    { TARGET_FLUSHO,  TARGET_FLUSHO,  FLUSHO,  FLUSHO },
    { TARGET_PENDIN,  TARGET_PENDIN,  PENDIN,  PENDIN },
    { TARGET_IEXTEN,  TARGET_IEXTEN,  IEXTEN,  IEXTEN },
    { TARGET_EXTPROC, TARGET_EXTPROC, EXTPROC, EXTPROC },
};

struct IOCTLEntry;

typedef abi_long do_ioctl_fn(const struct IOCTLEntry *ie, uint8_t *buf_temp,
                int fd, abi_long cmd, abi_long arg);

struct IOCTLEntry {
    unsigned int target_cmd;
    unsigned int host_cmd;
    const char *name;
    int access;
    do_ioctl_fn *do_ioctl;
    const argtype arg_type[5];
};
typedef struct IOCTLEntry IOCTLEntry;

#endif /* BSD_IOCTL_H */
