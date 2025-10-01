#!/usr/bin/env bash
set -euo pipefail
MODE="${1:-coverage}"
JOBS="${JOBS:-$(nproc || echo 2)}"
echo "[docker-ci] mode=$MODE jobs=$JOBS"

# Clean so coverage flags definitely take effect
make distclean >/dev/null 2>&1 || true
find . -name '*.gcno' -o -name '*.gcda' -delete || true

mkdir -p m4
if [ -f autogen.sh ]; then ./autogen.sh; else autoreconf -fvi; fi

# Configure normally; we will OVERRIDE at make-time (highest precedence)
export CC=gcc
./configure --enable-silent-rules

# Force coverage flags at make-time (overrides Makefile definitions)
MAKE_CFLAGS="--coverage -O0 -g"
MAKE_LDFLAGS="--coverage"

echo "[docker-ci] building (verbose) with forced flags…"
( set -o pipefail; make V=1 -j"$JOBS" CFLAGS="$MAKE_CFLAGS" LDFLAGS="$MAKE_LDFLAGS" 2>&1 | tee /tmp/build.log )

# Quick proof we actually compiled with coverage
echo "[docker-ci] sample compile lines that include coverage flags:"
grep -E -- '--coverage|-fprofile-arcs|-ftest-coverage' /tmp/build.log | head -n 10 || true

make check CFLAGS="$MAKE_CFLAGS" LDFLAGS="$MAKE_LDFLAGS" || true

# ---- Run the right binary (libtool wrapper if present) ----
AXEL_BIN="./src/axel"
if [ -x src/.libs/lt-axel ]; then AXEL_BIN="src/.libs/lt-axel"; fi
echo "[docker-ci] using binary: $AXEL_BIN"

# Local HTTP server so we exercise networking paths (no internet)
mkdir -p /tmp/axel_cov_svr
dd if=/dev/zero of=/tmp/axel_cov_svr/test.bin bs=1024 count=32 status=none
python3 -m http.server --directory /tmp/axel_cov_svr 8000 >/tmp/axel_http.log 2>&1 &
SVR_PID=$!; sleep 0.8

set +e
"$AXEL_BIN" -n 1 -o /tmp/axel_out.bin "http://127.0.0.1:8000/test.bin" >/dev/null 2>&1
AXEL_RC=$?
set -e

kill $SVR_PID 2>/dev/null || true; wait $SVR_PID 2>/dev/null || true
echo "[docker-ci] axel exit code: $AXEL_RC (non-zero is OK for coverage)"

# Also ping help/version
"$AXEL_BIN" --help >/dev/null 2>&1 || true
"$AXEL_BIN" --version >/dev/null 2>&1 || true

# ---- Inspect coverage artifacts ----
GCNO_COUNT=$(find . -name '*.gcno' | wc -l || echo 0)
GCDA_COUNT=$(find . -name '*.gcda' | wc -l || echo 0)
echo "[docker-ci] .gcno count: $GCNO_COUNT"
echo "[docker-ci] .gcda count: $GCDA_COUNT"
echo "[docker-ci] .gcno under src/ and src/.libs/:"
find src -name '*.gcno' -print 2>/dev/null || true
find src/.libs -name '*.gcno' -print 2>/dev/null || true
echo "[docker-ci] .gcda under src/ and src/.libs/:"
find src -name '*.gcda' -print 2>/dev/null || true
find src/.libs -name '*.gcda' -print 2>/dev/null || true

# If nothing was generated, stop with a clear message
if [ "${GCNO_COUNT}" -eq 0 ]; then
  echo "::error ::No .gcno files were generated. Make-time CFLAGS override didn’t take."
  echo "Next step: we’ll inject AM_CFLAGS/AM_LDFLAGS in Makefile.am for src/ targets."
  exit 2
fi

# Build list of object dirs that contain gcno/gcda
OBJ_DIRS=$(find . -type f \( -name '*.gcno' -o -name '*.gcda' \) -printf '%h\n' | sort -u)
[ -z "$OBJ_DIRS" ] && OBJ_DIRS="."

ROOT="$(pwd)"
mkdir -p coverage

# XML
gcovr -r "$ROOT" \
  $(for d in $OBJ_DIRS; do printf -- "--object-directory %q " "$d"; done) \
  --filter "$ROOT/src/.*" \
  --exclude '(^|/)?tests/.*' \
  --exclude '.*conftest.*' \
  --xml-pretty -o coverage/coverage.xml \
  --delete \
  --gcov-ignore-errors source_not_found \
  --gcov-ignore-errors no_working_dir_found

# HTML
gcovr -r "$ROOT" \
  $(for d in $OBJ_DIRS; do printf -- "--object-directory %q " "$d"; done) \
  --filter "$ROOT/src/.*" \
  --exclude '(^|/)?tests/.*' \
  --exclude '.*conftest.*' \
  --html --html-details -o coverage/index.html \
  --gcov-ignore-errors source_not_found \
  --gcov-ignore-errors no_working_dir_found

echo "[docker-ci] Coverage report: coverage/index.html"
