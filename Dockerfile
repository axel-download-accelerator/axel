# Dockerfile
FROM ubuntu:24.04

# Base tools + libs (note: txt2man is included)
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential automake libtool pkg-config \
    gettext autopoint autoconf-archive \
    gcovr lcov git wget xz-utils m4 txt2man \
    libssl-dev libgnutls28-dev libidn2-0-dev zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*

# Install Autoconf 2.72 from source (ubuntu 24.04 has 2.71)
ENV AUTOCONF_VERSION=2.72
RUN wget -q https://ftp.gnu.org/gnu/autoconf/autoconf-${AUTOCONF_VERSION}.tar.xz \
  && tar -xf autoconf-${AUTOCONF_VERSION}.tar.xz \
  && cd autoconf-${AUTOCONF_VERSION} \
  && ./configure \
  && make -j"$(nproc)" \
  && make install \
  && cd .. \
  && rm -rf autoconf-${AUTOCONF_VERSION} autoconf-${AUTOCONF_VERSION}.tar.xz

WORKDIR /work
CMD ["bash"]
