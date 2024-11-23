/*
 *  FreeBSD signal system call shims
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

#ifndef FREEBSD_OS_SIGNAL_H
#define FREEBSD_OS_SIGNAL_H

/* pdkill(2) */
static inline abi_long do_freebsd_pdkill(abi_long arg1, abi_long arg2)
{
#if !defined(__linux__)

    return get_errno(pdkill(arg1, arg2));
#else
    abort();
#endif
}

#endif /* FREEBSD_OS_SIGNAL_H */
