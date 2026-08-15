#!/bin/sh
# SPDX-FileCopyrightText: Copyright 2026 Ismael Luceno <ismael@iodev.co.uk>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# test/file-length.sh -- file and function length sanity check.
#
# A file nobody can hold in their head stops being read, and a layer
# violation hides best in the longest file of its layer.  The same is true
# one scale down: a 400-line function is where an unsequenced side effect or
# a missing early return survives review.  Headers cap at 500 lines, sources
# at 1000, and any single function at 150.  The numbers are arbitrary; the
# point is that crossing one has to be a deliberate act rather than a slow
# accretion.
#
# Every result reports the measurement, not just the verdict.  A file at 980
# lines passes today and is worth knowing about; a bare "ok" hides that.
# A failing function is named and located *in the test description*, not in a
# diagnostic: a TAP consumer is free to drop comments, and tap-prettify does,
# so "longest function is 158 lines" was a failure nobody could act on
# without rerunning the suite by hand.
#
# Every line, result and diagnostic alike, is prefixed with what this suite
# measures, ahead of the path it is about.  Several shell suites report paths
# into one merged TAP stream, and "src/text.c is 1200 lines" says neither
# which of them measured it nor what was being measured.
#
# Functions over FUNCTION_NOTICE lines are listed as TAP diagnostics at the
# end, longest first -- advisory, not failures, and the natural worklist when
# splitting something up.
#
# Two tests per file, not one per limit and not one per function: a single
# aggregate assertion reports the same "not ok" whether one file is over or
# twenty are, and buries the names in diagnostics the runner does not count.
# Per-file results make each offender its own failure.  Functions stop at
# per-file granularity deliberately -- one test per function would renumber
# the whole plan every time a static helper is added, where per-file numbering
# only moves when files do.  Files are ordered by path for the same reason.
#
# THE FUNCTION SCAN IS A HEURISTIC, not a C parser.  It keys on the layout
# this codebase actually uses: a definition begins at column 0 and its closing
# brace is a bare "}" at column 0.  Known blind spots, all of them safe in the
# sense that they under-report rather than fail a file wrongly:
#
#   - A body written entirely on the definition line is skipped: the semicolon
#     marks it as a declaration.  Nothing one line long can breach a 150-line
#     limit.
#   - A function whose closing brace is indented is never closed, so it is
#     not reported at all.  The count of files this happened to is printed
#     as a diagnostic, since a nonzero count means the heuristic broke rather
#     than that the tree is clean.
#   - A body defined by a macro is measured from the macro invocation, which
#     is what TEST(name) in the test suites relies on.
#
# Usage:  file-length.sh [srcroot]
#         srcroot defaults to $SRCTOP, then to the current directory.
#
# Env:    HEADER_MAX, SOURCE_MAX      per-file limits
#         FUNCTION_MAX                per-function limit
#         FUNCTION_NOTICE             report, but do not fail, above this
#
# TAP goes to stderr, as test/harness.h does, so tap-run treats this like
# any other suite.  Exit 0 when everything is within its limit, 1 otherwise.

set -u

root=${1:-${SRCTOP:-.}}
header_max=${HEADER_MAX:-500}
source_max=${SOURCE_MAX:-1000}
function_max=${FUNCTION_MAX:-150}
function_notice=${FUNCTION_NOTICE:-50}

# Prefixed to every message, ahead of the path it concerns.  A literal rather
# than $0: the point is to say what is being measured, and a filename cannot.
pfx='file length:'

TAB=$(printf '\t')
NL='
'

n=0
passed=0
failed=0

tmp=${TMPDIR:-/tmp}/axel-file-length.$$
fns=${TMPDIR:-/tmp}/axel-func-length.$$
trap 'rm -f "$tmp" "$fns"' EXIT
trap 'rm -f "$tmp" "$fns"; exit 1' HUP INT TERM

# Files exempt from the limits.  Keep this list short and justify every
# entry: an exemption is a decision not to split a file, not a free pass.
exempt() {
    case $1 in
    */config.h) return 0 ;;   # written by config.status
    esac
    return 1
}

# Functions over the limit that the run tolerates: measured, reported, and
# counted as TODO instead of failing.  Each line is "path:function", separated
# by ${NL}.  Removing one is the goal; adding one is not.  Empty is the state
# to keep it in.
grandfathered=""

# collect <max> <find-expression>... -- "lines TAB max TAB path" per match
collect() {
    max=$1
    shift

    find "$root" \
        \( -name .git -o -name _build -o -name autom4te.cache \
           -o -name .deps \) -prune -o \
        -type f \( "$@" \) -print |
    while IFS= read -r f; do
        exempt "$f" && continue
        printf '%s\t%s\t%s\n' \
            "$(wc -l < "$f" | tr -d '[:space:]')" "$max" "${f#"$root"/}"
    done
}

if [ ! -d "$root" ]; then
    printf '\n1..0 # %s no source root at %s\n' "$pfx" "$root" >&2
    exit 1
fi

{
    collect "$header_max" -name '*.h'
    collect "$source_max" -name '*.c'
} | sort -t"$TAB" -k3,3 > "$tmp"

total=$(wc -l < "$tmp" | tr -d '[:space:]')
ngrand=$(printf '%s\n' "$grandfathered" | grep -c '[^[:space:]]')

# Two tests per file -- its length and its longest function -- plus one per
# grandfathered function.
printf '\n1..%s\n' "$((total * 2 + ngrand))" >&2

if [ "$total" -eq 0 ]; then
    printf '# %s no headers or sources found under %s\n' "$pfx" "$root" >&2
    exit 1
fi

# ── function scan ────────────────────────────────────────────────────────────
# Reads the file list itself rather than taking paths as arguments, so a path
# containing whitespace cannot be split into two.  Emits one record per
# function: "lines TAB path TAB startline TAB name".
awk -v root="$root" -v listf="$tmp" -v pfx="$pfx" '
function strip_attr(s,   i, head, rest, n, c, depth) {
    # __attribute__((...)) sits between the storage class and the return
    # type, and its own parentheses would otherwise be mistaken for the
    # argument list.  Balanced-paren scan, since the payload nests.
    while ((i = index(s, "__attribute__")) > 0) {
        head = substr(s, 1, i - 1)
        rest = substr(s, i + 13)
        sub(/^[ \t]*/, "", rest)
        if (substr(rest, 1, 1) != "(") return head " " rest
        depth = 0
        for (n = 1; n <= length(rest); n++) {
            c = substr(rest, n, 1)
            if (c == "(") depth++
            else if (c == ")") { depth--; if (depth == 0) break }
        }
        s = head " " substr(rest, n + 1)
    }
    return s
}
function fname(s,   i, t, arg) {
    i = index(s, "(")
    if (i == 0) return "?"
    t = substr(s, 1, i - 1)
    sub(/[ \t]+$/, "", t)
    sub(/^.*[^A-Za-z0-9_]/, "", t)      # keep the last identifier
    if (t == "") return "?"
    # An all-caps token is a macro that expands to a definition; report its
    # argument, so the suites read as TEST(name) rather than 200 x TEST.
    if (t ~ /^[A-Z_][A-Z0-9_]*$/) {
        arg = substr(s, i + 1)
        i = index(arg, ")")
        if (i > 1) {
            arg = substr(arg, 1, i - 1)
            sub(/^[ \t]+/, "", arg)
            sub(/[ \t]+$/, "", arg)
            if (arg ~ /^[A-Za-z_][A-Za-z0-9_]*$/ && arg != "void")
                return t "(" arg ")"
        }
    }
    return t
}
function scan(rel,   full, line, nr, cand, sig, infunc, fstart, fnm, t, s) {
    full = root "/" rel
    nr = 0; cand = 0; sig = ""; infunc = 0
    while ((getline line < full) > 0) {
        nr++
        t = line
        sub(/[ \t]*\/\*.*$/, "", t)     # trailing or whole-line comment
        sub(/[ \t]*\/\/.*$/, "", t)
        sub(/[ \t]*$/, "", t)

        if (infunc) {
            # A bare closing brace only.  "} else {" at column 0 would end
            # the function early, and "} while (0)" is a macro, not a body.
            if (t ~ /^[}];?$/) {
                printf "%d\t%s\t%d\t%s\n", nr - fstart + 1, rel, fstart, fnm
                infunc = 0
            }
            continue
        }

        # A blank line or a preprocessor directive cannot be part of a
        # definition being accumulated.
        if (t == "" || t ~ /^[ \t]*[#]/) { cand = 0; sig = ""; continue }

        if (t ~ /^[A-Za-z_]/) {
            if (cand) sig = sig " " t
            else { cand = nr; sig = t }
        } else if (cand) {
            sig = sig " " t             # continuation of a wrapped signature
        } else continue

        # A semicolon means a prototype, a declaration, or a body written
        # entirely on one line.  None of them opens a block worth measuring.
        if (index(sig, ";") > 0) { cand = 0; sig = ""; continue }

        if (t !~ /[{]$/) continue

        s = strip_attr(sig)
        sub(/^[ \t]+/, "", s)
        # Needs an argument list, and an "=" makes it an initialiser.  The
        # paren test alone rejects "struct Foo {" and "extern \"C\" {", so the
        # keyword list only has to cover control flow.
        if (index(s, "(") > 0 && index(s, "=") == 0 &&
            s !~ /^(typedef|return|if|else|for|while|do|switch|case|default)[^A-Za-z0-9_]/) {
            infunc = 1
            fstart = cand
            fnm = fname(s)
        }
        cand = 0; sig = ""
    }
    close(full)
    if (infunc) unterminated++
}
BEGIN {
    FS = "\t"
    while ((getline rec < listf) > 0) {
        split(rec, a, FS)
        scan(a[3])
    }
    close(listf)
    if (unterminated > 0)
        printf "# %s note: %d file(s) ended inside a function; the scan is heuristic\n", \
               pfx, unterminated > "/dev/stderr"
}
' > "$fns"

# ── census ───────────────────────────────────────────────────────────────────
awk -F"$TAB" -v pfx="$pfx" '
    $1 > longest { longest = $1; path = $3 }
    END { printf "# %s %d files checked, longest %s at %d lines\n",
                 pfx, NR, path, longest }
' "$tmp" >&2

awk -F"$TAB" -v pfx="$pfx" '
    { if ($1 > longest) { longest = $1; where = $2 ":" $3; what = $4 } }
    END {
        if (NR == 0) { printf "# %s no functions found\n", pfx; exit }
        printf "# %s %d functions scanned, longest %s at %s (%d lines)\n",
               pfx, NR, what, where, longest
    }
' "$fns" >&2

# ── results ──────────────────────────────────────────────────────────────────
# Redirect rather than pipe: a `... | while` runs in a subshell, and the
# counters it increments are discarded when that subshell exits.
while IFS="$TAB" read -r lines max path; do
    n=$((n + 1))
    if [ "$lines" -le "$max" ]; then
        printf 'ok %d %s %s is %s lines (limit %s)\n' \
            "$n" "$pfx" "$path" "$lines" "$max" >&2
        passed=$((passed + 1))
    else
        printf 'not ok %d %s %s is %s lines (limit %s)\n' \
            "$n" "$pfx" "$path" "$lines" "$max" >&2
        failed=$((failed + 1))
    fi

    # count TAB longest TAB startline TAB name, for this file alone, with the
    # grandfathered functions left out of the maximum: they have a test of
    # their own below, and counting them here would hide every other function
    # in the same file behind one that is already known about.
    fstat=$(awk -F"$TAB" -v p="$path" -v grand="$grandfathered" '
        BEGIN { split(grand, g, "\n"); for (i in g) old[g[i]] = 1 }
        $2 == p { if (($2 ":" $4) in old) next
                  c++; if ($1 > m) { m = $1; l = $3; nm = $4 } }
        END { printf "%d\t%d\t%d\t%s\n", c + 0, m + 0, l + 0,
                     (nm == "" ? "-" : nm) }
    ' "$fns")
    nfun=${fstat%%"$TAB"*};  rest=${fstat#*"$TAB"}
    fmax=${rest%%"$TAB"*};   rest=${rest#*"$TAB"}
    fline=${rest%%"$TAB"*}
    fnm=${rest#*"$TAB"}

    n=$((n + 1))
    if [ "$nfun" -eq 0 ]; then
        printf 'ok %d %s %s defines no functions\n' "$n" "$pfx" "$path" >&2
        passed=$((passed + 1))
    elif [ "$fmax" -le "$function_max" ]; then
        printf 'ok %d %s %s longest function is %s at line %s (%s lines, limit %s)\n' \
            "$n" "$pfx" "$path" "$fnm" "$fline" "$fmax" "$function_max" >&2
        passed=$((passed + 1))
    else
        # Every offender, not just the longest: the description below can
        # only name one, and a file that is over twice is over twice.
        awk -F"$TAB" -v p="$path" -v lim="$function_max" -v pfx="$pfx" \
            -v grand="$grandfathered" '
            BEGIN { split(grand, g, "\n"); for (i in g) old[g[i]] = 1 }
            $2 == p && $1 > lim && !(($2 ":" $4) in old) {
                printf "#   %s %s:%s  %s  %s lines, limit %s\n",
                       pfx, $2, $3, $4, $1, lim
            }
        ' "$fns" >&2
        printf 'not ok %d %s %s longest function is %s at line %s (%s lines, limit %s)\n' \
            "$n" "$pfx" "$path" "$fnm" "$fline" "$fmax" "$function_max" >&2
        failed=$((failed + 1))
    fi
done < "$tmp"

# ── the grandfathered functions ──────────────────────────────────────────────
# One test each, always TODO, so the plan counts them and the summary shows
# how many are left.  A function that has come back under the limit -- or that
# no longer exists under that name -- reports an unexpected pass, which is the
# suite asking for its own list to be shortened.
printf '%s\n' "$grandfathered" | while IFS= read -r g; do
    case $g in '') continue ;; esac
    gpath=${g%%:*}
    gfunc=${g#*:}
    n=$((n + 1))

    glines=$(awk -F"$TAB" -v p="$gpath" -v f="$gfunc" '
        $2 == p && $4 == f && $1 > m { m = $1 }
        END { print m + 0 }
    ' "$fns")

    if [ "$glines" -gt "$function_max" ]; then
        printf 'not ok %d %s %s:%s is %s lines (limit %s) # TODO known long function, split it\n' \
            "$n" "$pfx" "$gpath" "$gfunc" "$glines" "$function_max" >&2
    elif [ "$glines" -eq 0 ]; then
        printf 'ok %d %s %s:%s is gone # TODO drop it from the grandfathered list\n' \
            "$n" "$pfx" "$gpath" "$gfunc" >&2
    else
        printf 'ok %d %s %s:%s is down to %s lines # TODO drop it from the grandfathered list\n' \
            "$n" "$pfx" "$gpath" "$gfunc" "$glines" >&2
    fi
done

# The loop above runs in a subshell -- it is fed by a pipe -- so it cannot
# hand back the numbers it printed.  Nothing after it needs them except the
# count, which is known.
n=$((n + ngrand))

# ── advisory: long but permitted functions ───────────────────────────────────
# Last, so it is the first thing visible after a run, and so a long list does
# not push the results off screen.  Includes the ones that failed above: this
# is a census of long functions, not a second failure report.
nlong=$(awk -F"$TAB" -v k="$function_notice" '$1 > k { c++ } END { print c + 0 }' "$fns")
if [ "$nlong" -gt 0 ]; then
    printf '# %s %d function(s) over %s lines, longest first:\n' \
        "$pfx" "$nlong" "$function_notice" >&2
    sort -t"$TAB" -k1,1nr "$fns" |
    awk -F"$TAB" -v k="$function_notice" -v pfx="$pfx" '
        $1 > k { printf "#   %s %5d  %s:%s  %s\n", pfx, $1, $2, $3, $4 }
    ' >&2
fi

printf '# %s %d passed  %d failed  %d grandfathered\n' \
    "$pfx" "$passed" "$failed" "$ngrand" >&2

exit $((failed > 0))
