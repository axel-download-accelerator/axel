#!/bin/bash

# for SSL/TLS support(OpenSSL, LibreSSL or compatible)
if [[ "$@" == "ssl" ]]; then
    sudo apt install libssl-dev
# extra dependencies for building from snapshots
elif [[ "$@" == "snapshot" ]]; then
    sudo apt install \
    autoconf-archive autoconf automake \
    autopoint txt2man
else
    # install default dependencies
    sudo apt install \
    gettext pkg-config \
    build-essential autoconf autoconf-archive \
    automake autopoint txt2man
fi