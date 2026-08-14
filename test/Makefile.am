# One binary per suite: harness.h keeps its registry in file-scope statics,
# so two suites linked together would leave one of them unreachable.
TEST_SUITES = test/netrc

# tap-run drives the suites itself, so automake's own driver stays out of it
check_PROGRAMS = $(TEST_SUITES) test/tap-run

EXTRA_DIST += test/tap-prettify
CLEANFILES = test/tap.log

test_netrc_SOURCES = \
	test/harness.h \
	test/netrc.c \
	src/netrc.c
test_netrc_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/src
test_netrc_CFLAGS = $(AM_CFLAGS) $(PTHREAD_CFLAGS)
test_netrc_LDADD = $(LIBOBJS) $(PTHREAD_LIBS)

test_tap_run_SOURCES = test/tap-run.c

# Straight down a pipe, so tap-prettify draws the run as it happens rather
# than after it; it ends the pipeline, so its exit status is the one make
# sees, and it fails whenever tap-run reported a failure, a suite that died,
# or a plan that came up short.  The raw TAP stays in test/tap.log for
# whoever has to read the run later.
#
# Run a subset with, e.g., make check TEST_SUITES=test/netrc
check-local: $(check_PROGRAMS)
	@test/tap-run $(TEST_SUITES) | tee test/tap.log \
		| $(AWK) -f $(top_srcdir)/test/tap-prettify
