# syntax=docker/dockerfile:1.7-labs

ARG BASE_IMAGE="debian:sid-slim"
FROM ${BASE_IMAGE} AS build
LABEL maintainer="Maksym Sobolyev <sobomax@FreeBSD.org>"

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_INSTALL="apt-get install --no-install-recommends -y"
ARG APT_UPDATE="apt-get -y update -qq"
RUN ${APT_UPDATE}
ARG CLANG_VER=19
ARG BUILD_PKGS="libelf-dev llvm-${CLANG_VER} clang-${CLANG_VER} python-is-python3 \
  python3-pip python3-setuptools ninja-build pkg-config libglib2.0-dev git \
  libbsd-dev"
WORKDIR /src
RUN --mount=type=bind,source=bsd-user/scripts/ci/pre-build.sh,target=pre-build.sh \
  ./pre-build.sh
RUN ${APT_INSTALL} ${BUILD_PKGS}
ARG TARGETPLATFORM
ARG BASE_IMAGE
RUN --mount=type=bind,target=.,rw \
  eval `./bsd-user/scripts/ci/get-arch-buildargs platformopts` && \
  bsd-user/scripts/ci/build.sh
