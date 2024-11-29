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

static const typeof(NLDLY) HOST_NLDLY = NLDLY;
static const typeof(CRDLY) HOST_CRDLY = CRDLY;
static const typeof(BSDLY) HOST_BSDLY = BSDLY;
static const typeof(FFDLY) HOST_FFDLY = FFDLY;
static const typeof(CBAUD) HOST_CBAUD = CBAUD;
static const typeof(CRTSCTS) HOST_CRTSCTS = CRTSCTS;

abi_long do_bsd_ioctl(int fd, abi_long cmd, abi_long arg);
void init_bsd_ioctl(void);

static const bitmask_transtbl iflag_tbl[] = {
    { IGNBRK, IGNBRK, HOST_IGNBRK, HOST_IGNBRK },
    { BRKINT, BRKINT, HOST_BRKINT, HOST_BRKINT },
    { IGNPAR, IGNPAR, HOST_IGNPAR, HOST_IGNPAR },
    { PARMRK, PARMRK, HOST_PARMRK, HOST_PARMRK },
    { INPCK, INPCK, HOST_INPCK, HOST_INPCK },
    { ISTRIP, ISTRIP, HOST_ISTRIP, HOST_ISTRIP },
    { INLCR, INLCR, HOST_INLCR, HOST_INLCR },
    { IGNCR, IGNCR, HOST_IGNCR, HOST_IGNCR },
    { ICRNL, ICRNL, HOST_ICRNL, HOST_ICRNL },
    { IXON, IXON, HOST_IXON, HOST_IXON },
    { IXANY, IXANY, HOST_IXANY, HOST_IXANY },
    { IXOFF, IXOFF, HOST_IXOFF, HOST_IXOFF },
    { IMAXBEL, IMAXBEL, HOST_IMAXBEL, HOST_IMAXBEL },
    { IUTF8, IUTF8, HOST_IUTF8, HOST_IUTF8},
};

static const bitmask_transtbl oflag_tbl[] = {
    { OPOST, OPOST, HOST_OPOST, HOST_OPOST },
    { ONLCR, ONLCR, HOST_ONLCR, HOST_ONLCR },
    { OCRNL, OCRNL, HOST_OCRNL, HOST_OCRNL },
    { ONOCR, ONOCR, HOST_ONOCR, HOST_ONOCR },
    { ONLRET, ONLRET, HOST_ONLRET, HOST_ONLRET },
    { NL0, NL0, HOST_NLDLY, HOST_NL0 },
    { NL1, NL1, HOST_NLDLY, HOST_NL1 },
    { CR0, CR0, HOST_CRDLY, HOST_CR0 },
    { CR1, CR1, HOST_CRDLY, HOST_CR1 },
    { CR2, CR2, HOST_CRDLY, HOST_CR2 },
    { CR3, CR3, HOST_CRDLY, HOST_CR3 },
    { TABDLY, TAB0, HOST_TABDLY, HOST_TAB0 },
    { TABDLY, TAB1, HOST_TABDLY, HOST_TAB1 },
    { TABDLY, TAB2, HOST_TABDLY, HOST_TAB2 },
    { TABDLY, TAB3, HOST_TABDLY, HOST_TAB3 },
    { BS0, BS0, HOST_BSDLY, HOST_BS0 },
    { BS1, BS1, HOST_BSDLY, HOST_BS1 },
    { FFDLY, FF0, HOST_FFDLY, HOST_FF0 },
    { FFDLY, FF1, HOST_FFDLY, HOST_FF1 },
};

static const bitmask_transtbl cflag_tbl[] = {
    { B0, B0, HOST_CBAUD, HOST_B0 },
    { B50, B50, HOST_CBAUD, HOST_B50 },
    { B75, B75, HOST_CBAUD, HOST_B75 },
    { B110, B110, HOST_CBAUD, HOST_B110 },
    { B134, B134, HOST_CBAUD, HOST_B134 },
    { B150, B150, HOST_CBAUD, HOST_B150 },
    { B200, B200, HOST_CBAUD, HOST_B200 },
    { B300, B300, HOST_CBAUD, HOST_B300 },
    { B600, B600, HOST_CBAUD, HOST_B600 },
    { B1200, B1200, HOST_CBAUD, HOST_B1200 },
    { B1800, B1800, HOST_CBAUD, HOST_B1800 },
    { B2400, B2400, HOST_CBAUD, HOST_B2400 },
    { B4800, B4800, HOST_CBAUD, HOST_B4800 },
    { B9600, B9600, HOST_CBAUD, HOST_B9600 },
    { B19200, B19200, HOST_CBAUD, HOST_B19200 },
    { B38400, B38400, HOST_CBAUD, HOST_B38400 },
    { B57600, B57600, HOST_CBAUD, HOST_B57600 },
    { B115200, B115200, HOST_CBAUD, HOST_B115200 },
    { B230400, B230400, HOST_CBAUD, HOST_B230400 },
    { B460800, B460800, HOST_CBAUD, HOST_B460800 },
    { CSIZE, CS5, HOST_CSIZE, HOST_CS5 },
    { CSIZE, CS6, HOST_CSIZE, HOST_CS6 },
    { CSIZE, CS7, HOST_CSIZE, HOST_CS7 },
    { CSIZE, CS8, HOST_CSIZE, HOST_CS8 },
    { CSTOPB, CSTOPB, HOST_CSTOPB, HOST_CSTOPB },
    { CREAD, CREAD, HOST_CREAD, HOST_CREAD },
    { PARENB, PARENB, HOST_PARENB, HOST_PARENB },
    { PARODD, PARODD, HOST_PARODD, HOST_PARODD },
    { HUPCL, HUPCL, HOST_HUPCL, HOST_HUPCL },
    { CLOCAL, CLOCAL, HOST_CLOCAL, HOST_CLOCAL },
    { CRTSCTS, CRTSCTS, HOST_CRTSCTS, HOST_CRTSCTS },
};

static const bitmask_transtbl lflag_tbl[] = {
    { ISIG, ISIG, HOST_ISIG, HOST_ISIG },
    { ICANON, ICANON, HOST_ICANON, HOST_ICANON },
    { ECHO, ECHO, HOST_ECHO, HOST_ECHO },
    { ECHOE, ECHOE, HOST_ECHOE, HOST_ECHOE },
    { ECHOK, ECHOK, HOST_ECHOK, HOST_ECHOK },
    { ECHONL, ECHONL, HOST_ECHONL, HOST_ECHONL },
    { NOFLSH, NOFLSH, HOST_NOFLSH, HOST_NOFLSH },
    { TOSTOP, TOSTOP, HOST_TOSTOP, HOST_TOSTOP },
    { ECHOCTL, ECHOCTL, HOST_ECHOCTL, HOST_ECHOCTL },
    { ECHOPRT, ECHOPRT, HOST_ECHOPRT, HOST_ECHOPRT },
    { ECHOKE, ECHOKE, HOST_ECHOKE, HOST_ECHOKE },
    { FLUSHO, FLUSHO, HOST_FLUSHO, HOST_FLUSHO },
    { PENDIN, PENDIN, HOST_PENDIN, HOST_PENDIN },
    { IEXTEN, IEXTEN, HOST_IEXTEN, HOST_IEXTEN },
    { EXTPROC, EXTPROC, HOST_EXTPROC, HOST_EXTPROC},
};

#endif /* BSD_IOCTL_H */
