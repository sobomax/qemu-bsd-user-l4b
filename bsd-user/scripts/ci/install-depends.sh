#!/bin/sh

set -e

install_libkqueue() {
  git clone https://github.com/sobomax/libkqueue.git /tmp/libkqueue
  cmake -B /tmp/libkqueue/build -S /tmp/libkqueue -DCMAKE_C_COMPILER=clang-${CLANG_VER} -DENABLE_SHARED=OFF ${CMAKE_PARAMS}
  make -C /tmp/libkqueue/build all install
}

install_libkqueue
