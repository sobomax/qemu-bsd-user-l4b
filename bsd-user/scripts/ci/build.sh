#!/bin/sh

set -e
set -x

BDIR=build_ci

_EXTRA_CFLAGS="-O1 -pipe -ffunction-sections -fdata-sections"
if [ ! -z "${EXTRA_CFLAGS}" ]
then
  _EXTRA_CFLAGS="${_EXTRA_CFLAGS} ${EXTRA_CFLAGS}"
fi
_EXTRA_LDFLAGS="-Wl,--gc-sections"
if [ ! -z "${EXTRA_LDFLAGS}" ]
then
  _EXTRA_LDLAGS="${_EXTRA_LDFLAGS} ${EXTRA_LDFLAGS}"
fi

mkdir ${BDIR}
cd ${BDIR}
../configure --disable-system --static --target-list=x86_64-bsd-user \
  --extra-cflags="${_EXTRA_CFLAGS}" --cc="clang-${CLANG_VER}" \
  --extra-ldflags="${_EXTRA_LDLAGS}"
ninja
