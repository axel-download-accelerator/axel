// SPDX-FileCopyrightText: Copyright 2026 Ismael Luceno <ismael@iodev.co.uk>
// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * test/tap-run.c – run test binaries in parallel, emit multiplexed TAP.
 *
 * Usage: ./test/tap-run test/netrc test/whatever ...
 *        ./test/tap-run $(TEST_SUITES) | ./test/tap-prettify
 *
 * Each line written to stdout has the form:
 *     SUITE<TAB>LINE
 *
 * If a binary exits non-zero a sentinel line is emitted:
 *     __status__<TAB>SUITE<TAB>EXIT_CODE
 *
 * A suite that cannot be started at all gets a synthetic one-test plan and
 * a "not ok" ahead of that sentinel, so it is counted as a failure rather
 * than as no tests.  Without it a run where nothing could be spawned --
 * `make clean all test` with no rule building the suites, say -- summarises
 * as all-passing, which is the one outcome a test runner must never report
 * wrongly.
 *
 * Compile with: gcc -std=c99 -O2 -o tap-run tap-run.c
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L   /* for asprintf, strdup, posix_spawn, etc. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <spawn.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <sys/wait.h>

/* POSIX.1-2008 declares environ in <unistd.h> */

#ifndef IOV_MAX
# define IOV_MAX 1024
#endif

#define CHUNK_SIZE 4096
#define IOV_INIT 64

/* Exit code reported for a suite we could not even start.  Matches the
 * convention the old execvp-failure path used. */
#define SPAWN_FAILED_CODE 127

typedef struct child {
    pid_t pid;
    int fd;                 /* read end of pipe, -1 when closed */
    int spawn_failed;       /* posix_spawnp() itself failed for this suite */
    char *suite;            /* basename of the binary */
    char *prefix;           /* suite + "\t" (allocated) */
    size_t prefix_len;
    char *buf;              /* dynamic buffer for partial data */
    size_t buf_len;
    size_t buf_cap;
} child_t;

/* A stable newline we can point an iovec at. */
static char s_newline = '\n';

/* Helper: grow an iovec array and append a new entry. */
static void iovec_append(struct iovec **iov, int *cnt, int *cap,
                         void *base, size_t len) {
    if (*cnt >= *cap) {
        *cap *= 2;
        *iov = realloc(*iov, *cap * sizeof(struct iovec));
        if (!*iov) {
            perror("realloc iovec");
            exit(1);
        }
    }
    (*iov)[*cnt].iov_base = base;
    (*iov)[*cnt].iov_len  = len;
    (*cnt)++;
}

/* Write an iovec array to stdout, respecting IOV_MAX per call. */
static void write_iovecs(struct iovec *iov, int cnt) {
    int written = 0;
    while (written < cnt) {
        int chunk = cnt - written;
        if (chunk > IOV_MAX) chunk = IOV_MAX;
        ssize_t ret = writev(STDOUT_FILENO, iov + written, chunk);
        if (ret < 0) {
            if (errno == EPIPE)     /* stdout closed */
                exit(0);
            perror("writev");
            return;
        }
        written += chunk;
    }
}

/* Emit one line into the multiplexed stream, attributed to this suite. */
static void emit_suite_line(const child_t *c, const char *text) {
    struct iovec iov[3];
    iov[0].iov_base = c->prefix;
    iov[0].iov_len  = c->prefix_len;
    iov[1].iov_base = (void *)text;
    iov[1].iov_len  = strlen(text);
    iov[2].iov_base = &s_newline;
    iov[2].iov_len  = 1;
    writev(STDOUT_FILENO, iov, 3);   /* ignore errors */
}

/* Report a suite that never ran as a failing one-test plan.  Emitted into
 * the TAP stream rather than onto stderr: a consumer counts what it can
 * parse, and 26 unparsed stderr lines summarise as a clean run. */
static void emit_spawn_failure(const child_t *c, int err) {
    char msg[256];
    snprintf(msg, sizeof(msg), "not ok 1 - %s failed to start: %s",
             c->suite, strerror(err));
    emit_suite_line(c, "1..1");
    emit_suite_line(c, msg);
}

/* Start one suite.  Returns 0 on success (including a recorded spawn
 * failure, which is not fatal to the run) and -1 on unrecoverable error. */
static int child_spawn(child_t *c, char *path) {
    char *suite = strrchr(path, '/');
    suite = suite ? suite + 1 : path;

    c->suite = strdup(suite);
    if (!c->suite) {
        perror("strdup");
        return -1;
    }
    if (asprintf(&c->prefix, "%s\t", c->suite) < 0) {
        perror("asprintf");
        return -1;
    }
    c->prefix_len = strlen(c->prefix);
    c->buf = NULL;
    c->buf_len = 0;
    c->buf_cap = 0;

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return -1;
    }

    pid_t pid;
    char *argv_child[] = {path, NULL};
    posix_spawn_file_actions_t facts;
    posix_spawn_file_actions_init(&facts);
    posix_spawn_file_actions_addclose(&facts, pipefd[0]);
    posix_spawn_file_actions_adddup2(&facts, pipefd[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&facts, STDOUT_FILENO, STDERR_FILENO);
    posix_spawn_file_actions_addclose(&facts, pipefd[1]);

    int rc = posix_spawnp(&pid, path, &facts, NULL, argv_child, environ);
    posix_spawn_file_actions_destroy(&facts);
    if (rc != 0) {
        /* Don't abort the whole run over one missing/unrunnable suite:
         * record it as already-finished with a synthetic exit code and let
         * every other suite still run. */
        close(pipefd[0]);
        close(pipefd[1]);
        c->pid = -1;
        c->fd  = -1;
        c->spawn_failed = 1;
        emit_spawn_failure(c, rc);
        return 0;
    }

    /* parent */
    close(pipefd[1]);
    /* set non-blocking so we never block on read after select */
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    c->pid = pid;
    c->fd  = pipefd[0];
    return 0;
}

/* Emit a trailing partial line (no newline seen before EOF) and release
 * the child's buffer. */
static void child_flush_tail(child_t *c) {
    if (c->buf_len > 0) {
        struct iovec iov[3];
        iov[0].iov_base = c->prefix;
        iov[0].iov_len  = c->prefix_len;
        iov[1].iov_base = c->buf;
        iov[1].iov_len  = c->buf_len;
        iov[2].iov_base = &s_newline;
        iov[2].iov_len  = 1;
        writev(STDOUT_FILENO, iov, 3);   /* ignore errors */
    }
    free(c->buf);
    c->buf = NULL;
    c->buf_len = 0;
    c->buf_cap = 0;
}

/* Append freshly read bytes to the child's line buffer. */
static void child_append(child_t *c, const char *data, size_t n) {
    size_t needed = c->buf_len + n;
    if (needed + 1 > c->buf_cap) {
        c->buf_cap = needed + CHUNK_SIZE;
        char *newbuf = realloc(c->buf, c->buf_cap);
        if (!newbuf) {
            perror("realloc buffer");
            exit(1);
        }
        c->buf = newbuf;
    }
    memcpy(c->buf + c->buf_len, data, n);
    c->buf_len += n;
}

/* Emit every complete line held in the child's buffer, keeping the
 * incomplete tail for the next read. */
static void child_emit_lines(child_t *c) {
    char *start = c->buf;
    size_t remain = c->buf_len;

    int iov_cnt = 0, iov_cap = IOV_INIT;
    struct iovec *iov = malloc(iov_cap * sizeof(struct iovec));
    if (!iov) {
        perror("malloc iovec");
        exit(1);
    }

    while (remain > 0) {
        char *nl = memchr(start, '\n', remain);
        if (!nl) break;                 /* no more complete lines */
        size_t line_len = nl - start;   /* length without newline */

        /* prefix (suite + tab) */
        iovec_append(&iov, &iov_cnt, &iov_cap, c->prefix, c->prefix_len);
        /* line data (without newline) */
        iovec_append(&iov, &iov_cnt, &iov_cap, start, line_len);
        /* our own newline */
        iovec_append(&iov, &iov_cnt, &iov_cap, &s_newline, 1);

        /* advance past the original newline */
        start = nl + 1;
        remain -= (line_len + 1);
    }

    if (iov_cnt > 0)
        write_iovecs(iov, iov_cnt);
    free(iov);

    /* keep the incomplete tail in the buffer */
    if (remain > 0)
        memmove(c->buf, start, remain);
    c->buf_len = remain;
}

/* Service one readable child.  Returns 1 if the child's fd was closed
 * (EOF or fatal read error), 0 otherwise. */
static int child_handle_read(child_t *c) {
    char tmp[CHUNK_SIZE];
    ssize_t nr = read(c->fd, tmp, sizeof(tmp));

    if (nr < 0) {
        if (errno == EAGAIN
#if EWOULDBLOCK != EAGAIN
            || errno == EWOULDBLOCK
#endif
           ) return 0;
        perror("read");
        /* treat read error as fatal: close fd and forget child */
        close(c->fd);
        c->fd = -1;
        return 1;
    }
    if (nr == 0) {          /* EOF */
        close(c->fd);
        c->fd = -1;
        child_flush_tail(c);
        return 1;
    }

    child_append(c, tmp, (size_t)nr);
    child_emit_lines(c);
    return 0;
}

/* Main I/O loop: read from children, split lines, write batches. */
static void run_loop(child_t *children, int n) {
    int active = 0;
    int maxfd = 0;
    for (int i = 0; i < n; i++) {
        if (children[i].fd != -1) active++;
        if (children[i].fd > maxfd) maxfd = children[i].fd;
    }

    while (active > 0) {
        fd_set readfds;
        FD_ZERO(&readfds);
        for (int i = 0; i < n; i++) {
            if (children[i].fd != -1)
                FD_SET(children[i].fd, &readfds);
        }

        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        for (int i = 0; i < n; i++) {
            if (children[i].fd == -1) continue;
            if (!FD_ISSET(children[i].fd, &readfds)) continue;
            if (child_handle_read(&children[i]))
                active--;
        }
    }
}

/* Wait for one child and return its exit code in shell convention. */
static int child_reap(child_t *c) {
    if (c->spawn_failed)
        return SPAWN_FAILED_CODE;

    int status;
    pid_t ret;
    do {
        ret = waitpid(c->pid, &status, 0);
    } while (ret == -1 && errno == EINTR);
    if (ret == -1) {
        perror("waitpid");
        return 0;
    }
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);   /* common shell convention */
    return 0;
}

static void emit_status(const char *suite, int code) {
    static const char status_prefix[] = "__status__\t";
    static const char tab = '\t';
    char code_str[32];
    snprintf(code_str, sizeof(code_str), "%d", code);

    struct iovec iov[5];
    iov[0].iov_base = (void *)status_prefix;
    iov[0].iov_len  = sizeof(status_prefix) - 1;
    iov[1].iov_base = (void *)suite;
    iov[1].iov_len  = strlen(suite);
    iov[2].iov_base = (void *)&tab;
    iov[2].iov_len  = 1;
    iov[3].iov_base = code_str;
    iov[3].iov_len  = strlen(code_str);
    iov[4].iov_base = &s_newline;
    iov[4].iov_len  = 1;
    writev(STDOUT_FILENO, iov, 5);
}

/* Reap all children, emit __status__ lines for non-zero exits, free state.
 * Returns non-zero if any suite failed. */
static int reap_all(child_t *children, int n) {
    int failed = 0;
    for (int i = 0; i < n; i++) {
        int code = child_reap(&children[i]);
        if (code != 0) {
            failed = 1;
            emit_status(children[i].suite, code);
        }
        free(children[i].suite);
        free(children[i].prefix);
        free(children[i].buf);
    }
    return failed;
}

int main(int argc, char **argv) {
    if (argc < 2)
        return 0;

    signal(SIGPIPE, SIG_IGN);          /* mimic Python's BrokenPipeError */

    int n = argc - 1;
    child_t *children = calloc(n, sizeof(child_t));
    if (!children) {
        perror("calloc");
        return 1;
    }

    /* Launch all children, each with its own pipe (stdout+stderr merged). */
    for (int i = 0; i < n; i++) {
        if (child_spawn(&children[i], argv[i + 1]) < 0)
            return 1;
    }

    run_loop(children, n);

    int failed = reap_all(children, n);
    free(children);
    return failed ? 1 : 0;
}
