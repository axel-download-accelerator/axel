#!/bin/sh
# SPDX-FileCopyrightText: Copyright 2026 Ismael Luceno <ismael@iodev.co.uk>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# test/file-map.sh -- every source is built, every build input exists.
#
# Two failure modes, one cause: the Makefile.am fragments are hand-maintained
# lists of what to compile, and nothing checks them against the tree.
#
#   - An orphaned source.  A .c file nobody builds compiles nowhere, so it
#     drifts out of sync with the headers it includes and is discovered only
#     when someone finally adds it to a fragment and it no longer builds.  A
#     new test suite that was never added to TEST_SUITES is the same bug
#     wearing a test's clothes: it passes by not running.
#   - A dangling reference.  A name in _SOURCES that matches no file is a
#     hard error from automake, but one inside a conditional arm that is not
#     taken, or in a variable this project happens not to expand, is not.  A
#     reference to a file that does not exist is not always a build error,
#     which is exactly why it needs a test.
#
# One test per source for the first, one test per fragment for the second.
# Per-fragment rather than per-token, for the reason file-length.sh keeps
# functions at per-file granularity: token-level numbering would renumber the
# whole plan every time a line moves.
#
# CONFIGURE.AC COUNTS AS A FRAGMENT.  lib/strlcpy.c, lib/strlcat.c and
# lib/ASN1_STRING_get0_data.c appear in no Makefile.am at all: they reach the
# link through $(LIBOBJS), and what names them is AC_REPLACE_FUNCS.  A scan
# that read only the Makefile.am files would report all three as orphans.
#
# Every line, result and diagnostic alike, is prefixed with what this suite
# checks, ahead of the path it is about.  Several shell suites report paths
# into one merged TAP stream, and a bare path says neither which of them
# looked at it nor what it was looking for.
#
# THE SCAN IS TEXTUAL, not a make parser.  It cannot expand a variable, so:
#
#   - A source is "referenced" if its stem appears as a whole word anywhere
#     in a fragment, with comments stripped.  That accepts foo.c, foo.o, the
#     bare `test/foo` of a program target and the bare `foo` of a replaced
#     libc function, and it over-accepts: a stem named in prose counts.
#     Over-accepting is the safe direction -- this test exists to catch the
#     file nobody mentioned at all.
#   - A reference is a token ending in .c or .o after $(...) and ${...} are
#     blanked.  Pattern rules, suffix rules and anything containing % or *
#     name no particular file and drop out with them.
#   - Resolution is by basename against every .c in the tree, so a fragment
#     may name an object without saying which directory builds it.
#
# Only the .am files are read, never the Makefile or Makefile.in that
# automake writes from them: those are generated, absent from a fresh
# checkout, and present twice over in an in-tree build.
#
# Usage:  file-map.sh [srcroot]
#         srcroot defaults to $SRCTOP, then to the current directory.
#
# TAP goes to stderr, as test/harness.h does, so tap-run treats this like
# any other suite.  Exit 0 when the map is consistent, 1 otherwise.

set -u

root=${1:-${SRCTOP:-.}}

# Prefixed to every message, ahead of the path it concerns.  A literal rather
# than $0: the point is to say what is being checked, and a filename cannot.
pfx='build map:'

n=0
passed=0
failed=0

srcs=${TMPDIR:-/tmp}/axel-file-map-src.$$
mks=${TMPDIR:-/tmp}/axel-file-map-mk.$$
bases=${TMPDIR:-/tmp}/axel-file-map-base.$$
toks=${TMPDIR:-/tmp}/axel-file-map-tok.$$
trap 'rm -f "$srcs" "$mks" "$bases" "$toks"' EXIT
trap 'rm -f "$srcs" "$mks" "$bases" "$toks"; exit 1' HUP INT TERM

# Sources that deliberately live outside the build.  Keep this empty if at
# all possible: an entry here is a file the tree carries and never compiles,
# which is the thing this suite was written to find.
unbuilt() {
    case $1 in
    @NOTHING@) return 0 ;;   # placeholder; the case needs one arm
    esac
    return 1
}

# Directories that hold no source of ours: the build tree distcheck puts
# inside the distribution, autoconf's cache, and automake's dependency files.
prune='( -name .git -o -name _build -o -name autom4te.cache -o -name .deps )'

if [ ! -d "$root" ]; then
    printf '\n1..0 # %s no source root at %s\n' "$pfx" "$root" >&2
    exit 1
fi

# shellcheck disable=SC2086  # $prune is a find expression, not a filename
find "$root" $prune -prune -o -type f -name '*.c' -print |
while IFS= read -r f; do
    rel=${f#"$root"/}
    unbuilt "$rel" && continue
    printf '%s\n' "$rel"
done | sort > "$srcs"

# shellcheck disable=SC2086  # as above
find "$root" $prune -prune -o \
    -type f \( -name 'Makefile.am' -o -name '*.mk' -o -name 'configure.ac' \) \
    -print |
while IFS= read -r f; do
    printf '%s\n' "${f#"$root"/}"
done | sort > "$mks"

awk -F/ '{ print $NF }' "$srcs" | sort -u > "$bases"

nsrc=$(wc -l < "$srcs" | tr -d '[:space:]')
nmk=$(wc -l < "$mks" | tr -d '[:space:]')

printf '\n1..%s\n' "$((nsrc + nmk))" >&2

if [ "$nsrc" -eq 0 ] || [ "$nmk" -eq 0 ]; then
    printf '# %s %d sources and %d build fragments under %s\n' \
        "$pfx" "$nsrc" "$nmk" "$root" >&2
    exit 1
fi

printf '# %s %d sources, %d build fragments\n' "$pfx" "$nsrc" "$nmk" >&2

# ── every source is referenced ───────────────────────────────────────────────
# Redirect rather than pipe: a `... | while` runs in a subshell, and the
# counters it increments are discarded when that subshell exits.
while IFS= read -r rel; do
    n=$((n + 1))
    stem=${rel##*/}
    stem=${stem%.c}
    # Escape the regex metacharacters a filename can plausibly contain.
    pat=$(printf '%s' "$stem" | sed 's/[.[*^$\\]/\\&/g')

    # Two passes, so the fragment reported is the one that means it.  A
    # reference naming the file -- "src/abuf.c", or the "abuf.o" of an
    # object -- comes first; the bare stem is the fallback, since that is all
    # AC_REPLACE_FUNCS leaves of lib/strlcpy.c, and since any fragment merely
    # mentioning the name in prose satisfies it too.
    hit=
    for how in names mentions; do
        case $how in
        names)    re="(^|[^A-Za-z0-9_])${pat}\\.[co]($|[^A-Za-z0-9_])" ;;
        mentions) re="(^|[^A-Za-z0-9_])${pat}($|[^A-Za-z0-9_])" ;;
        esac
        while IFS= read -r mk; do
            if sed 's/#.*//' "$root/$mk" | grep -qE "$re"; then
                hit=$mk
                break
            fi
        done < "$mks"
        [ -n "$hit" ] && break
    done

    if [ -n "$hit" ] && [ "$how" = names ]; then
        printf 'ok %d %s %s is built by %s\n' "$n" "$pfx" "$rel" "$hit" >&2
        passed=$((passed + 1))
    elif [ -n "$hit" ]; then
        printf 'ok %d %s %s is named by %s\n' "$n" "$pfx" "$rel" "$hit" >&2
        passed=$((passed + 1))
    else
        printf '#   %s %s is in the tree but named by no build fragment\n' \
            "$pfx" "$rel" >&2
        printf 'not ok %d %s %s is referenced by no build fragment\n' \
            "$n" "$pfx" "$rel" >&2
        failed=$((failed + 1))
    fi
done < "$srcs"

# ── every reference resolves ─────────────────────────────────────────────────
while IFS= read -r mk; do
    n=$((n + 1))

    awk '
    {
        line = $0
        sub(/#.*/, "", line)                 # comment
        gsub(/\$[({][^)}]*[)}]/, " ", line)  # ${var} and $(var)
        gsub(/\$./, " ", line)               # $@, $<, $*
        k = split(line, t, /[^A-Za-z0-9_.\/+-]+/)
        for (i = 1; i <= k; i++) {
            tok = t[i]
            # Must start with a name character, so ".c.o:" and a bare ".o"
            # left behind by a stripped "%" are not mistaken for files.
            if (tok ~ /^[A-Za-z0-9_].*\.[co]$/)
                print tok
        }
    }
    ' "$root/$mk" | sort -u > "$toks"

    missing=0
    while IFS= read -r tok; do
        [ -n "$tok" ] || continue
        base=${tok##*/}
        case $base in
        *.o) src=${base%.o}.c ;;
        *)   src=$base ;;
        esac
        grep -qxF "$src" "$bases" && continue
        printf '#   %s %s names %s, and no %s exists in the tree\n' \
            "$pfx" "$mk" "$tok" "$src" >&2
        missing=$((missing + 1))
    done < "$toks"

    ntok=$(wc -l < "$toks" | tr -d '[:space:]')
    if [ "$missing" -eq 0 ]; then
        printf 'ok %d %s %s: all %s source references resolve\n' \
            "$n" "$pfx" "$mk" "$ntok" >&2
        passed=$((passed + 1))
    else
        printf 'not ok %d %s %s: %s of %s source references resolve to nothing\n' \
            "$n" "$pfx" "$mk" "$missing" "$ntok" >&2
        failed=$((failed + 1))
    fi
done < "$mks"

printf '# %s %d passed  %d failed\n' "$pfx" "$passed" "$failed" >&2

exit $((failed > 0))
