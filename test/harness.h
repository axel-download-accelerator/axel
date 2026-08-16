// SPDX-FileCopyrightText: Copyright 2026 Ismael Luceno <ismael@iodev.co.uk>
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * test/harness.h - Test harness.
 *
 * API:
 *   TEST(name) { body }       - define a test
 *
 *   Assertions come in two families with identical diagnostics:
 *
 *     ASSERT_x   record the failure and return from the test
 *     CHECK_x    record the failure and carry on
 *
 *     ASSERT(cond)            CHECK(cond)
 *     ASSERT_EQ(a, b)         CHECK_EQ(a, b)        integer ==
 *     ASSERT_NE(a, b)         CHECK_NE(a, b)        integer !=
 *     ASSERT_GT(a, b)         CHECK_GT(a, b)        integer >
 *     ASSERT_LT(a, b)         CHECK_LT(a, b)        integer <
 *     ASSERT_GE(a, b)         CHECK_GE(a, b)        integer >=
 *     ASSERT_LE(a, b)         CHECK_LE(a, b)        integer <=
 *     ASSERT_STR(a, b)        CHECK_STR(a, b)       strcmp == 0
 *     ASSERT_LSTR(p, len, s)  CHECK_LSTR(p, len, s) (ptr,len) vs literal
 *     ASSERT_NULL(p)          CHECK_NULL(p)
 *     ASSERT_NOTNULL(p)       CHECK_NOTNULL(p)
 *     ASSERT_OK(r)            CHECK_OK(r)           r == 0
 *     ASSERT_FLOAT_EQ/NE/GT/LT/GE/LE   CHECK_FLOAT_EQ/NE/GT/LT/GE/LE
 *
 *   ASSERT_x is defined as CHECK_x followed by a return, so the two can
 *   never disagree about what a comparison means or how it is reported.
 *
 *   Use ASSERT_x by default.  Use CHECK_x where the remainder of the test
 *   body must still run:
 *     - a fixture needs freeing, and an early return would leak it;
 *     - the later assertions are still informative once one has failed.
 *
 *   A CHECK inside a long loop reports once per iteration, so a single bug
 *   becomes hundreds of lines.  Record the first bad index and assert on
 *   that after the loop instead.
 *
 *   In main():
 *     REGISTER_DESC(name, "...")  register a test, giving the sentence that
 *                                 becomes its TAP description
 *     REGISTER(name)              register a test with no description; the
 *                                 bare identifier is reported instead
 *     RUN_ALL()                   run every registered test
 *     DONE()                      returns 0 if all passed, 1 otherwise
 *
 *   Prefer REGISTER_DESC.  A TAP consumer prints the description and
 *   nothing else, so `not ok 12 sm_remove_clears_has` tells a reader which
 *   function ran but not what it was supposed to do - and an identifier
 *   cannot be made to say it, being a name rather than a sentence.  Write
 *   the expected behaviour in the indicative: "remove deletes the entry and
 *   drops the count", not "test remove" and not "should delete the entry".
 *   The identifier stays in the source as the thing to grep for, and a
 *   failure's file and line locate it.
 *
 * TDD notes:
 *   Inner-loop tests exercise internal contracts (units).
 *   Outer-loop tests exercise user-visible behaviour end-to-end.
 *   Both use the same TEST() macro; the distinction is in naming.
 *
 * Timeouts:
 *   Each test runs under a SIGALRM watchdog (HARNESS_TIMEOUT_SECONDS,
 *   default 10).  A test that hangs is reported as a normal failure and the
 *   run continues, instead of wedging the whole suite with no output - which
 *   matters for regression tests covering genuine infinite loops, where the
 *   pre-fix behaviour is precisely "never returns".
 *
 *   Build with -DHARNESS_TIMEOUT_SECONDS=0 to compile the watchdog out: it
 *   needs POSIX signals, and it will fire while you sit at a breakpoint
 *   under a debugger.
 */

#ifndef HARNESS_H
#define HARNESS_H

/* alarm()/sigaction() are hidden under -std=c99 without this.  Only set it
 * if the including translation unit has not already chosen a level. */
#ifndef HARNESS_TIMEOUT_SECONDS
#define HARNESS_TIMEOUT_SECONDS 10
#endif

#if HARNESS_TIMEOUT_SECONDS > 0
#  if !defined(_POSIX_C_SOURCE) && !defined(_GNU_SOURCE) && \
      !defined(_DEFAULT_SOURCE) && !defined(_BSD_SOURCE)
#    define _POSIX_C_SOURCE 200809L
#  endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HARNESS_TIMEOUT_SECONDS > 0
#  include <signal.h>
#  include <setjmp.h>
#  include <unistd.h>
#endif

/* ------------------------------------------------------------------ */
/* Registry                                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *tname;              /* renamed to avoid clash with REGISTER(name) */
    const char *tdesc;              /* NULL when registered without one           */
    void (*tfn)(int *_h_f);         /* renamed to avoid clash */
} _HTest;

#define _H_MAX   256
static _HTest _h_tests[_H_MAX];
static int    _h_n      = 0;
static int    _h_passed = 0;
static int    _h_failed = 0;

/* ------------------------------------------------------------------ */
/* Per-test watchdog                                                    */
/* ------------------------------------------------------------------ */

#if HARNESS_TIMEOUT_SECONDS > 0

static sigjmp_buf            _h_timeout_jmp;
static volatile sig_atomic_t _h_timed_out;

/* siglongjmp out of a signal handler abandons whatever the test was holding
 * - heap blocks, an open display connection, a locked surface.  A timed-out
 * run therefore leaks, and any later test relying on shared global state may
 * be unreliable.  That is an acceptable trade here: the alternative is the
 * suite producing no result at all. */
static void _h_on_alarm(int sig) __attribute__((noreturn));
static void _h_on_alarm(int sig) {
    (void)sig;
    _h_timed_out = 1;
    siglongjmp(_h_timeout_jmp, 1);
}

static void _h_watchdog_arm(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = _h_on_alarm;
    sigemptyset(&sa.sa_mask);
    /* No SA_RESTART: the handler never returns, so it cannot matter, but
     * leaving it off also means a blocking syscall cannot swallow the jump. */
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);
    alarm((unsigned)HARNESS_TIMEOUT_SECONDS);
}

static void _h_watchdog_disarm(void) {
    alarm(0);
    signal(SIGALRM, SIG_DFL);
}

#endif /* HARNESS_TIMEOUT_SECONDS > 0 */

/* ------------------------------------------------------------------ */
/* Non-fatal checks - record the failure, keep going                    */
/* ------------------------------------------------------------------ */

#define CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "    FAIL  %s:%d  %s\n", \
                __FILE__, __LINE__, #cond); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_EQ(a, b) do { \
    long long _va = (long long)(a), _vb = (long long)(b); \
    if (_va != _vb) { \
        fprintf(stderr, "    FAIL  %s:%d  EQ  %s=%lld  %s=%lld\n", \
                __FILE__, __LINE__, #a, _va, #b, _vb); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_NE(a, b) do { \
    long long _va = (long long)(a), _vb = (long long)(b); \
    if (_va == _vb) { \
        fprintf(stderr, "    FAIL  %s:%d  NE  %s == %s == %lld\n", \
                __FILE__, __LINE__, #a, #b, _va); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_GT(a, b) do { \
    long long _va = (long long)(a), _vb = (long long)(b); \
    if (!(_va > _vb)) { \
        fprintf(stderr, "    FAIL  %s:%d  GT  %s=%lld  %s=%lld\n", \
                __FILE__, __LINE__, #a, _va, #b, _vb); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_LT(a, b) do { \
    long long _va = (long long)(a), _vb = (long long)(b); \
    if (!(_va < _vb)) { \
        fprintf(stderr, "    FAIL  %s:%d  LT  %s=%lld  %s=%lld\n", \
                __FILE__, __LINE__, #a, _va, #b, _vb); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_GE(a, b) do { \
    long long _va = (long long)(a), _vb = (long long)(b); \
    if (!(_va >= _vb)) { \
        fprintf(stderr, "    FAIL  %s:%d  GE  %s=%lld  %s=%lld\n", \
                __FILE__, __LINE__, #a, _va, #b, _vb); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_LE(a, b) do { \
    long long _va = (long long)(a), _vb = (long long)(b); \
    if (!(_va <= _vb)) { \
        fprintf(stderr, "    FAIL  %s:%d  LE  %s=%lld  %s=%lld\n", \
                __FILE__, __LINE__, #a, _va, #b, _vb); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_STR(a, b) do { \
    const char *_sa = (a), *_sb = (b); \
    if (!_sa || !_sb || strcmp(_sa, _sb) != 0) { \
        fprintf(stderr, "    FAIL  %s:%d  STR  \"%s\" != \"%s\"\n", \
                __FILE__, __LINE__, \
                _sa ? _sa : "(null)", _sb ? _sb : "(null)"); \
        (*_h_f)++; \
    } \
} while (0)

/* Compare a (pointer, length) pair against a C string literal. */
#define CHECK_LSTR(ptr, len, lit) do { \
    const char *_lp = (ptr); int _ll = (int)(len); \
    int _le = (int)strlen(lit); \
    if (!_lp || _ll != _le || strncmp(_lp, (lit), (size_t)_le) != 0) { \
        fprintf(stderr, "    FAIL  %s:%d  LSTR  \"%.*s\"(len=%d) != \"%s\"\n", \
                __FILE__, __LINE__, _ll, _lp ? _lp : "", _ll, (lit)); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_NULL(p) do { \
    if ((void *)(p) != NULL) { \
        fprintf(stderr, "    FAIL  %s:%d  NULL: %s\n", \
                __FILE__, __LINE__, #p); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_NOTNULL(p) do { \
    if ((void *)(p) == NULL) { \
        fprintf(stderr, "    FAIL  %s:%d  NOTNULL: %s\n", \
                __FILE__, __LINE__, #p); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_OK(r) do { \
    int _rok = (r); \
    if (_rok != 0) { \
        fprintf(stderr, "    FAIL  %s:%d  OK  got %d: %s\n", \
                __FILE__, __LINE__, _rok, #r); \
        (*_h_f)++; \
    } \
} while (0)

/* Float checks (1e-4f epsilon for EQ/NE; strict for ordering) */
#define CHECK_FLOAT_EQ(a, b) do { \
    float _fa = (float)(a), _fb = (float)(b); \
    float _d  = _fa - _fb; if (_d < 0) _d = -_d; \
    if (_d > 1e-4f) { \
        fprintf(stderr, "    FAIL  %s:%d  FLOAT_EQ  %s=%g  %s=%g\n", \
                __FILE__, __LINE__, #a, (double)_fa, #b, (double)_fb); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_FLOAT_NE(a, b) do { \
    float _fa = (float)(a), _fb = (float)(b); \
    float _d  = _fa - _fb; if (_d < 0) _d = -_d; \
    if (_d <= 1e-4f) { \
        fprintf(stderr, "    FAIL  %s:%d  FLOAT_NE  %s=%g == %s=%g\n", \
                __FILE__, __LINE__, #a, (double)_fa, #b, (double)_fb); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_FLOAT_GT(a, b) do { \
    float _fa = (float)(a), _fb = (float)(b); \
    if (!(_fa > _fb)) { \
        fprintf(stderr, "    FAIL  %s:%d  FLOAT_GT  %s=%g  %s=%g\n", \
                __FILE__, __LINE__, #a, (double)_fa, #b, (double)_fb); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_FLOAT_LT(a, b) do { \
    float _fa = (float)(a), _fb = (float)(b); \
    if (!(_fa < _fb)) { \
        fprintf(stderr, "    FAIL  %s:%d  FLOAT_LT  %s=%g  %s=%g\n", \
                __FILE__, __LINE__, #a, (double)_fa, #b, (double)_fb); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_FLOAT_GE(a, b) do { \
    float _fa = (float)(a), _fb = (float)(b); \
    if (!(_fa >= _fb)) { \
        fprintf(stderr, "    FAIL  %s:%d  FLOAT_GE  %s=%g  %s=%g\n", \
                __FILE__, __LINE__, #a, (double)_fa, #b, (double)_fb); \
        (*_h_f)++; \
    } \
} while (0)

#define CHECK_FLOAT_LE(a, b) do { \
    float _fa = (float)(a), _fb = (float)(b); \
    if (!(_fa <= _fb)) { \
        fprintf(stderr, "    FAIL  %s:%d  FLOAT_LE  %s=%g  %s=%g\n", \
                __FILE__, __LINE__, #a, (double)_fa, #b, (double)_fb); \
        (*_h_f)++; \
    } \
} while (0)

/* -------------------------------------------------------------------- */
/* Fatal assertions - the checks above, plus a return                   */
/*                                                                      */
/* Defined in terms of CHECK_x so there is exactly one implementation   */
/* of each comparison and one message format for it.  The failure       */
/* counter is the signal: if the check bumped it, the test stops.       */
/*                                                                      */
/* The comma in e.g. _H_FATAL(CHECK_EQ(a, b)) is inside CHECK_EQ's own  */
/* parentheses, so it is not an argument separator.                     */
/* -------------------------------------------------------------------- */

#define _H_FATAL(chk) do { \
    int _h_before = *_h_f; \
    chk; \
    if (*_h_f != _h_before) return; \
} while (0)

#define ASSERT(cond)            _H_FATAL(CHECK(cond))
#define ASSERT_EQ(a, b)         _H_FATAL(CHECK_EQ(a, b))
#define ASSERT_NE(a, b)         _H_FATAL(CHECK_NE(a, b))
#define ASSERT_GT(a, b)         _H_FATAL(CHECK_GT(a, b))
#define ASSERT_LT(a, b)         _H_FATAL(CHECK_LT(a, b))
#define ASSERT_GE(a, b)         _H_FATAL(CHECK_GE(a, b))
#define ASSERT_LE(a, b)         _H_FATAL(CHECK_LE(a, b))
#define ASSERT_STR(a, b)        _H_FATAL(CHECK_STR(a, b))
#define ASSERT_LSTR(p, len, s)  _H_FATAL(CHECK_LSTR(p, len, s))
#define ASSERT_NULL(p)          _H_FATAL(CHECK_NULL(p))
#define ASSERT_NOTNULL(p)       _H_FATAL(CHECK_NOTNULL(p))
#define ASSERT_OK(r)            _H_FATAL(CHECK_OK(r))

#define ASSERT_FLOAT_EQ(a, b)   _H_FATAL(CHECK_FLOAT_EQ(a, b))
#define ASSERT_FLOAT_NE(a, b)   _H_FATAL(CHECK_FLOAT_NE(a, b))
#define ASSERT_FLOAT_GT(a, b)   _H_FATAL(CHECK_FLOAT_GT(a, b))
#define ASSERT_FLOAT_LT(a, b)   _H_FATAL(CHECK_FLOAT_LT(a, b))
#define ASSERT_FLOAT_GE(a, b)   _H_FATAL(CHECK_FLOAT_GE(a, b))
#define ASSERT_FLOAT_LE(a, b)   _H_FATAL(CHECK_FLOAT_LE(a, b))

/* ------------------------------------------------------------------ */
/* Test definition                                                      */
/* ------------------------------------------------------------------ */

/* Each TEST(name) expands to a static function _hfn_name(int *_h_f). */
#define TEST(tst) \
    static void _hfn_##tst(int *_h_f); \
    static void _hfn_##tst(int *_h_f)

/* ------------------------------------------------------------------ */
/* Registration                                                         */
/* ------------------------------------------------------------------ */

#define REGISTER_DESC(tst, desc) do { \
    if (_h_n < _H_MAX) { \
        _h_tests[_h_n].tname = #tst; \
        _h_tests[_h_n].tdesc = (desc); \
        _h_tests[_h_n].tfn   = _hfn_##tst; \
        _h_n++; \
    } \
} while (0)

/* Reports the bare identifier.  No caller in this tree -- every suite gives
 * a description -- and kept anyway, because deleting it means renaming
 * REGISTER_DESC back to REGISTER in every suite: no behaviour change, and
 * one reviewable diff turned into an unreviewable one.  A suite written
 * against this header from outside the tree also keeps building. */
#define REGISTER(tst) REGISTER_DESC(tst, NULL)

/* ------------------------------------------------------------------ */
/* Runner                                                               */
/* ------------------------------------------------------------------ */

static void RUN_ALL(void) {
    fprintf(stderr, "\n1..%d\n", _h_n);
    for (int i = 0; i < _h_n; i++) {
        const char *what = _h_tests[i].tdesc ? _h_tests[i].tdesc
                                             : _h_tests[i].tname;
        int f = 0;
#if HARNESS_TIMEOUT_SECONDS > 0
        _h_timed_out = 0;
        if (sigsetjmp(_h_timeout_jmp, 1) == 0) {
            _h_watchdog_arm();
            _h_tests[i].tfn(&f);
            _h_watchdog_disarm();
        } else {
            _h_watchdog_disarm();
            fprintf(stderr, "    FAIL  timed out after %d s\n",
                    HARNESS_TIMEOUT_SECONDS);
            f++;
        }
#else
        _h_tests[i].tfn(&f);
#endif
        if (f == 0) {
            fprintf(stderr, "ok %d %s\n", i + 1, what);
            _h_passed++;
        } else {
            fprintf(stderr, "not ok %d %s\n", i + 1, what);
            _h_failed++;
        }
    }
}

static int DONE(void) {
    fprintf(stderr, "# %d passed  %d failed\n", _h_passed, _h_failed);
    return _h_failed > 0;
}

#endif /* HARNESS_H */
