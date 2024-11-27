=======================
BSD-USER 4 LINUX README
=======================

The bsd-user-4-linux fork of QEMU. Documentation can be found hosted online at
`<https://www.qemu.org/documentation/>`_ for the upstream project.

The bsd-user-4-linux ports the BSD user-mode emulation for the QEMU project,
so that it can run on Linux as well. There are a number of changes that need
to be upstreamed, however.

It is a "fork of a fork", based off excellent job done by Warner Losh & Co
fixing and improving bsd-user support.

Warner's project is available at `<https://github.com/qemu-bsd-user/qemu-bsd-user>`_.

Goal and Objective
==================

Implement enough support for the FreeBSD to be able to compile itself
on Linux using FreeBSD native toolchain.

Little bit down the road, ability to do a package building.

Getting Started
===============

To get started, you'd need a fairly recent Ubuntu. I am developing and
testing with 24.04, but other versions might do just fine.

.. code-block:: shell

  # apt update
  # apt install [TO BE DETERMINED]

Next, you'll need to clone this repo. I like to use the directory 'bsd-user'
for the fork.

.. code-block:: shell

  % mkdir git
  % cd git
  % git clone -b blitz git@github.com:sobomax/qemu-bsd-user-l4b.git bsd-user
  % cd bsd-user
  % mkdir 00-bsd-user
  % cd 00-bsd-user
  % ../configure --disable-system --static --target-list=x86_64-bsd-user
  % gmake

The above takes a little while to build. I disable the qemu-system-* binaries
since they take a longer time to build and aren't relevant to bsd-user. Other
than the args to configure, this is the standard way you build qemu.

Usage
=====

To use BSDlator, you would need to grab a recent FreeBSD binary release (I am
using FreeBSD 14.1, but other versions might work too) and unpack it
into some suitable directory within your Linux system.

Then you can run FreeBSD binary by providing the path to that directory as
an -L argument for a qemu, so it knows where to locate the libraries and other
files followed by the path to a binary and corresponding command line
arguments.

.. code-block:: shell

  % qemu-x86_64 -L /usr/lib/freebsd ~/.local/bin/ls -l

  do_bsd_lpathconf: stub
  ls: ./Makefile: No such file or directory
  total 13140
  lrwxrwxrwx   1 1000 sshrpc       31 Nov 23  2024 Makefile -> /tmp/qemu-bsd-user-l4b/Makefile
  -rw-rw-r--   1 1000 sshrpc     8028 Nov 25  2024 Makefile.mtest
  -rw-rw-r--   1 1000 sshrpc    86230 Nov 25  2024 Makefile.ninja
  -rw-rw-r--   1 1000 sshrpc      107 Nov 23  2024 Makefile.prereqs
  -rw-rw-r--   1 1000 sshrpc      690 Nov 23  2024 aarch64-linux-user-config-target.h
  -rw-rw-r--   1 1000 sshrpc      696 Nov 23  2024 aarch64_be-linux-user-config-target.h
  drwxrwxr-x   3 1000 sshrpc        3 Nov 23  2024 accel
  -rw-rw-r--   1 1000 sshrpc      555 Nov 23  2024 alpha-linux-user-config-target.h

Status
======

Most of the basic syscalls are working. Dynamic linking, shared library support, and so on.
Things that are missing at the moment:

* IPC and networking;
* Terminal and process control;
* ioctl()'s;
* Threading.
