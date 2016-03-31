#!/bin/sh
# Copyright 2016 Stephen Thirlwall

autoreconf --install "$@" || exit 1

echo "Now run ./configure, make, and make install."
