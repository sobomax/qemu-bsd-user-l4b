===============
BSD USER README
===============

The bsd-user fork of QEMU. Documentation can be found hosted online at
`<https://www.qemu.org/documentation/>`_ for the upstream project.

bsd-user fixes the BSD user-mode emulation for the QEMU project. There are a
number of changes that need to be upstreamed, however.

Getting Started
===============

To get started, create a FreeBSD VM with a ZFS root filesystem. This is detailed
fairly well in `<https://docs.freebsd.org/en/books/handbook/bsdinstall/>`_ so
I'll omit the details here. Make sure that the ZFS pool is named zpool. It will
make your life much easier. This is the default. You should install FreeBSD 13.2
release.

Next, once you've installed your system, set a root password and create an
account. It's better to work not as root, but you will need root from time to
time. I recommend that you install (# are commands as root, % are commands as user)

.. code-block:: shell

  # pkg bootstrap
  # pkg install git pkgconf bzip2 ninja bash gmake gsed gettext gnutls jpeg-turbo png sdl2 libxkbcommon mesa-libs zstd libslirp sndio python libproxy meson pixman bison
  # pkg install qemu-user-static
  # pkg install poudriere
  # rehash
  # sysrc qemu_user_static_enable=YES
  # service qemu_user_static start

This will install all the prerequisites to successfully build. Except the last
one, that will install FreeBSD's qemu-user-static package, which will setup
things so you can directly run the armv7 or other foreign binaries. More on what
you'll do with this later. Likewise with poudriere. It's not needed to build
bsd-user, but is used for testing it.

Next, you'll need to clone this repo. I like to use the directory 'bsd-user'
for the fork and 'qemu' for the upstream project. The rest 

.. code-block:: shell

  % mkdir git
  % cd git
  % git clone -b blitz  https://github.com/qemu-bsd-user/qemu-bsd-user.git bsd-user
  % git clone git@gitlab.com:qemu-project/qemu.git qemu
  % cd bsd-user
  % mkdir 00-bsd-user
  % cd 00-bsd-user
  % ../configure --disable-system --static
  % gmake
  % cd ../../qemu
  % mkdir 00-qemu
  % cd 00-qemu
  % ../configure --disable-system --static
  % gmake

The above takes a little while to build. I disable the qemu-system-* binaries
since they take a longer time to build and aren't relevant to bsd-user. Other
than the args to configure, this is the standard way you build qemu.

Setting up Pouduriere
====================

Poudriere is the standard way that FreeBSD builds package. It has a great many
feaetures one won't use when developing bsd-user, unless you are using it to
build package. There's a number of tutorials on poduriere online, so I won't
repeat them here. I will show how to build a jail, however

.. code-block:: shell

  # poudriere jail -c -j 132armv7 -a arm.armv7 -m git+ssh -v releng/13.2

Here we're building a armv7 tree. For armv7 in 13.2 and earlier, you have to
build from sources. This takes a while.

Note about packages
===================
These instructions have people install qemu-user-static so that the binmiscctl
commands are executed at boot. /usr/local/bin/qemu-$ARCH-user is copied over
(see below), so after installing it, I usually do the following:

.. code-block:: shell

  # cd /usr/local/bin
  # mv qemu-arm-static qemu-arm-static.3.1
  # ln -s qemu-arm-static.3.1 qemu-arm-static

so that the binary is a symlink. Later, when I want to test, I copy my qemu-arm
that I build (more on that later) into either
/usr/local/bin/qemu-arm-static.bsd-user or /usr/local/bin/qemu-arm-static.up
so I can do A/B testing more eaily by moving the symbolic link.

Starting the jail
=================

The following starts the jail and then shows how to jexec into it to get a shell
prompt:

.. code-block:: shell

  # poudriere jail -s -j 132armv7
  # jls
  <listing of the jails to get the jail number>
  # jexec X
  # 

This will test to make sure that you have qmeu-user-static installed
correctly. You should get a # prompt from our (somewhat old based on 3.1)
qemu-arm-static binary.

When Poudriere starts a jail, it copies a 'clean' snapshot so that any changes
to the jail aren't recorded. It also copies /usr/local/bin/qemu-$ARCH-user into
that tree. The $ARCH in this case is arm for armv7 binaries. This is why I
usually create a symlink: so I can move it as I test. One can also copy it into
the running jail directory, though I try not to do that since I like restarting
my jails when I change tests. One can also have multiple jails one leaves
running and just copy bsd-user or upstream binaries in as needed. Finally, if
you are just going to test qemu-arm for one binary, you can copy it directly
into the jail w/o updating the symlink so that all the other command work. We
recommend --static so that one can do this w/o needing to copy libraries over as
well.

Building Test Binaries Without The Jail
=======================================

Since you've created the jail, you have a 'sysroot' that you can use to build
binaries. Let's say you want to build hello-armv7 from hello.c.

.. code-block:: shell

 % cc -target freebsd-armv7 --sysroot /vidpool/qemu/jails/jails/131armv7 -o hello-armv7


Will do the trick.

Running Without The Jail
========================

Sometimes it is desirable to run qemu to test without running in the jail. You
will still need to create the jail, as outlined above, but you don't need to
start it. You'll need to get the 'root' of the jail for this step. Use
`poudriere jail -l` to get a list of all your jails, and to find the root

.. code-block:: shell

 # joudriere jail -l
 JAILNAME        VERSION                              ARCH      METHOD  TIMESTAMP           PATH
 131armv7        13.2-RC3 1302001 d9bf9d732           arm.armv7 git+ssh 2023-03-18 13:54:23 /vidpool/qemu/jails/jails/131armv7
 #

In this case, it's the PATH column.

You'll can test binaries either inside or outside the jail. You'll run qemu-user
directly to do this test. Let's say you have a 'hello world' binary that you're
trying to debug. For example, if you're debugging an arm binary using the above
jail:

.. code-block:: shell

 % cd qemu/00-qemu
 % <build-here>
 % qemu-arm -L /vidpool/qemu/jails/jails/131armv7 hello-arm

whill run it looking in the jail's root directory for all the dynamic parts of
the binary (ld-elf.so, libc.so, etc).

