# One binary per suite: harness.h keeps its registry in file-scope statics,
# so two suites linked together would leave one of them unreachable.
TEST_SUITES = test/netrc test/conf test/tcp

# Some properties of the tree are invisible to a program compiled from it:
# how long its files are, and whether they are compiled at all.  These suites
# read the source tree instead, so there is nothing to build and nothing a
# build directory would hold -- they run from $(top_srcdir), and find it
# again through SRCTOP, since tap-run passes a suite no arguments.  They emit
# the same TAP on stderr as test/harness.h, so tap-run needs to know nothing
# about the language.
SH_SUITES = \
	test/file-length.sh \
	test/file-map.sh

# tap-run drives the suites itself, so automake's own driver stays out of it
check_PROGRAMS = $(TEST_SUITES) test/tap-run

EXTRA_DIST += $(SH_SUITES) test/tap-prettify
CLEANFILES = test/tap.log

test_netrc_SOURCES = \
	test/harness.h \
	test/netrc.c \
	src/netrc.c
test_netrc_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src
test_netrc_CFLAGS = $(AM_CFLAGS) $(PTHREAD_CFLAGS)
test_netrc_LDADD = $(LIBOBJS) $(PTHREAD_LIBS)

test_conf_SOURCES = \
	test/harness.h \
	test/conf.c \
	src/conf.c \
	src/netrc.c
test_conf_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src
test_conf_CFLAGS = $(AM_CFLAGS) $(PTHREAD_CFLAGS)
test_conf_LDADD = $(LIBOBJS) $(LIBINTL) $(PTHREAD_LIBS)

test_tcp_SOURCES = \
	test/harness.h \
	test/tcp.c \
	src/tcp.c
test_tcp_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src
test_tcp_CFLAGS = $(AM_CFLAGS) $(PTHREAD_CFLAGS)
test_tcp_LDADD = $(LIBOBJS) $(LIBINTL) $(PTHREAD_LIBS)

if WITH_SSL
test_tcp_SOURCES += \
	src/dn-match.c \
	src/ssl.c \
	src/ssl_verify.c
test_tcp_LDADD += $(SSL_LIBS)
endif

test_tap_run_SOURCES = test/tap-run.c

# Straight down a pipe, so tap-prettify draws the run as it happens rather
# than after it; it ends the pipeline, so its exit status is the one make
# sees, and it fails whenever tap-run reported a failure, a suite that died,
# or a plan that came up short.  The raw TAP stays in test/tap.log for
# whoever has to read the run later.
#
# Run a subset with, e.g., make check TEST_SUITES=test/netrc SH_SUITES=
#
# The loop is what a `$(SH_SUITES:%=$(top_srcdir)/%)` would say in GNU
# make and nowhere else; the scripts have to be named relative to the source
# tree for EXTRA_DIST above, and absolutely for a build outside it.
check-local: $(check_PROGRAMS)
	@scripts=; for s in $(SH_SUITES); do \
		scripts="$$scripts $(abs_top_srcdir)/$$s"; \
	done; \
	SRCTOP='$(abs_top_srcdir)' \
	    test/tap-run $(TEST_SUITES) $$scripts | tee test/tap.log \
		| $(AWK) -f $(top_srcdir)/test/tap-prettify
