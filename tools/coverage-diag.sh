#!/usr/bin/env bash
set -euxo pipefail

echo "=== ENV ==="
date -Is || true
autoconf --version | head -n1 || true
automake --version | head -n1 || true
gcc --version | head -n1 || true
gcovr --version || true
lcov --version || true

echo "=== CLEAN ==="
make distclean || true
rm -rf coverage autom4te.cache config.log config.status config.h* aclocal.m4 || true
find . \( -name "*.o" -o -name "*.lo" -o -name "*.la" -o -name "*.a" -o -name "*.gcno" -o -name "*.gcda" -o -name ".deps" -o -name ".libs" \) -exec rm -rf {} + || true

echo "=== BOOTSTRAP ==="
mkdir -p m4
autoreconf -fvi

echo "=== CONFIGURE (force coverage at compiler level) ==="
CC="gcc --coverage" CFLAGS="-O0 -g" LDFLAGS="--coverage" ./configure --enable-silent-rules

echo "=== BUILD (verbose) ==="
set -o pipefail
make V=1 -j"$(nproc || echo 2)" 2>&1 | tee coverage-build.log
set +o pipefail

echo "=== SAMPLE COMPILE LINES (src/*.c) ==="
grep -E " -c .*src/.*\.c" coverage-build.log | head -n 40 || true

echo "=== MAKEFILE SNIPPET (src/Makefile.am) ==="
head -n 200 src/Makefile.am || true

echo "=== .gcno after build ==="
find . -name "*.gcno" -printf "%p -> %h\n" | sort || true

echo "=== SMOKE RUN (local HTTP) ==="
mkdir -p /tmp/axel_cov_svr
dd if=/dev/zero of=/tmp/axel_cov_svr/test.bin bs=1024 count=64 status=none
python3 -m http.server --directory /tmp/axel_cov_svr 8000 >/tmp/axel_http.log 2>&1 &
SVR_PID=$!; sleep 0.8
AXEL_BIN=./src/axel; [ -x src/.libs/lt-axel ] && AXEL_BIN=src/.libs/lt-axel
set +e
"$AXEL_BIN" -n 1 -o /tmp/axel_out.bin http://127.0.0.1:8000/test.bin >/dev/null 2>&1
"$AXEL_BIN" --help >/dev/null 2>&1
"$AXEL_BIN" --version >/dev/null 2>&1
set -e
kill $SVR_PID 2>/dev/null || true; wait $SVR_PID 2>/dev/null || true

echo "=== .gcno/.gcda counts (in repo) ==="
echo ".gcno: $(find . -name "*.gcno" | wc -l)   .gcda: $(find . -name "*.gcda" | wc -l)"

echo "=== TRY GCOV REDIRECT (GCOV_PREFIX) ==="
mkdir -p coverage/gcda
for STRIP in $(seq 0 10); do
  rm -rf coverage/gcda/* || true
  env GCOV_PREFIX=/work/coverage/gcda GCOV_PREFIX_STRIP=$STRIP \
    "$AXEL_BIN" -n 1 -o /tmp/axel_out2.bin http://127.0.0.1:8000/test.bin >/dev/null 2>&1 || true
  CNT=$(find coverage/gcda -name "*.gcda" | wc -l || echo 0)
  echo "[gcda] after STRIP=$STRIP -> $CNT files"
  [ "$CNT" -gt 0 ] && break
done
echo ".gcda redirected: $(find coverage/gcda -name "*.gcda" | wc -l || true)"

echo "=== REPORTS (gcovr + lcov) ==="
OBJ_DIRS="."
[ -d coverage/gcda ] && OBJ_DIRS="$OBJ_DIRS coverage/gcda"
mkdir -p coverage
gcovr -r . $(for d in $OBJ_DIRS; do printf -- "--object-directory %q " "$d"; done) \
      --filter "src/.*" --exclude ".*conftest.*" \
      --html --html-details -o coverage/gcovr-index.html \
      --gcov-ignore-errors source_not_found --gcov-ignore-errors no_working_dir_found || true
lcov --capture --directory . --output-file coverage/coverage.info --ignore-errors empty --no-external || true
genhtml coverage/coverage.info --output-directory coverage/lcov-html --title "axel coverage" --ignore-errors empty || true

echo "=== DONE ==="
date -Is || true
