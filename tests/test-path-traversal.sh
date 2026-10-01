#!/bin/sh

set -eu

srcdir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
top_srcdir="$(CDPATH= cd -- "$srcdir/.." && pwd)"
builddir="$(mktemp -d)"
binary="$builddir/test-path-traversal"

cleanup()
{
	rm -rf "$builddir"
}

trap cleanup EXIT HUP INT TERM

cc -ffunction-sections -fdata-sections -Wl,--gc-sections \
	-I"$srcdir" -I"$top_srcdir" -I"$top_srcdir/src" \
	-o "$binary" \
	"$srcdir/test-path-traversal.c" \
	"$top_srcdir/src/conn.c" \
	"$top_srcdir/src/http.c" \
	"$top_srcdir/lib/strlcpy.c" \
	"$top_srcdir/lib/strlcat.c"

"$binary"
