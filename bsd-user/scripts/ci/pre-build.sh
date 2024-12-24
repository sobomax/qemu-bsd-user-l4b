#!/bin/sh

set -e

install_clang() {
  local i=0
  while true
  do
    ${APT_INSTALL} curl gpg lsb-release ca-certificates && break
    test ${i} -gt 1 && exit 1
    ${APT_UPDATE}
    local i=$((${i} + 1))
  done
  MYREL="`lsb_release -c | grep ^Codename | awk '{print $2}'`"
  echo "deb [signed-by=/usr/share/keyrings/llvm.gpg] http://apt.llvm.org/${MYREL}/ llvm-toolchain-${MYREL}-${CLANG_VER} main" > /etc/apt/sources.list.d/llvm.list
  curl https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor > /usr/share/keyrings/llvm.gpg
  ${APT_UPDATE}
  apt-mark hold ca-certificates
}

install_clang
