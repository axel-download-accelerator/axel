# Autoconf for axel
AC_COPYRIGHT([
Copyright 2016      Joao Eriberto Mota Filho
Copyright 2016      Stephen Thirlwall
Copyright 2017      Antonio Quartulli
Copyright 2017-2020 Ismael Luceno
Copyright 2017      Vlad Glagolev

This file is under the same license as Axel.
])

AC_PREREQ([2.69])
PKG_PREREQ([0.29])

AXEL_PKG([axel], [https://github.com/axel-download-accelerator/axel/issues])
AXEL_VER_READ([VERSION])
AC_INIT

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/conf.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_LIBOBJ_DIR([lib])
AM_INIT_AUTOMAKE([subdir-objects])
PKG_PROG_PKG_CONFIG

AM_SILENT_RULES([yes])

AC_USE_SYSTEM_EXTENSIONS

# Compiler checks
AC_PROG_CC
AC_PROG_CC_C99
AS_IF([test "x$ac_cv_prog_cc_c99" = "xno"], [
    AC_MSG_ERROR([C99 compiler required])
])

# Compiler Warnings
AX_IS_RELEASE([git-directory])
AX_COMPILER_FLAGS()

# Debugging code
AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--enable-debug], [enable debugging, default: no]))
AS_IF([! test "x$enable_debug" = xyes ], [
    AC_DEFINE([NDEBUG], [1], [disable debugging code])
])

# GCC function attributes
AX_GCC_FUNC_ATTRIBUTE([format])

# Built-in functions
AX_GCC_BUILTIN([__builtin_clzll])

AS_IF([test "x$ax_cv_have___builtin_clzll" = xno], [
    AC_MSG_ERROR([Support for __builtin_clzll is required])
])


# Checks for other programs.
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([ \
	arpa/inet.h \
	fcntl.h \
	limits.h \
	locale.h \
	netdb.h \
	netinet/in.h \
	stdlib.h \
	sys/ioctl.h \
	sys/socket.h \
	sys/time.h \
	unistd.h \
],, [
    AC_MSG_ERROR([$ac_header is required.])
])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T

# Checks for library functions.
AC_CHECK_FUNCS([ \
	getaddrinfo \
	gettimeofday \
	inet_ntoa \
	malloc \
	memset \
	nanosleep \
	realloc \
	select \
	setlocale \
	socket \
	strcasecmp \
	strchr \
	strerror \
	strncasecmp \
	strpbrk \
	strrchr \
	strstr \
	strtoul \
],, [
    AC_MSG_ERROR([$ac_func is required; it should be part of libc.])
])

AC_REPLACE_FUNCS([ \
	strlcpy \
	strlcat \
])

# Optional (but included-by-default) ssl support
AC_ARG_WITH([ssl],
    AC_HELP_STRING([--without-ssl],[disable TLS support]), [], [with_ssl=yes])

AM_CONDITIONAL([WITH_SSL], [test "x$with_ssl" != xno])
AS_IF([test "x$with_ssl" != xno], [
    PKG_CHECK_MODULES([SSL], [openssl])
    AC_DEFINE([HAVE_SSL], [1], [SSL])
    save_LIBS="$LIBS"
    LIBS="$LIBS $SSL_LIBS"
    AC_REPLACE_FUNCS([ \
        ASN1_STRING_get0_data \
    ])
    LIBS="$save_LIBS"
], AC_MSG_NOTICE([SSL support disabled]))

# Add Gettext
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.11.1])

# Large file support
AC_SYS_LARGEFILE

# POSIX threads
AX_PTHREAD()

#### Definitions ####

# OS
AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED([ARCH], ["$host_os"], ["Define architecture"])

case "$host_os" in
darwin*)
    AC_DEFINE([_DARWIN_C_SOURCE], 1, [Define on Darwin to expose all libc features])
;;
solaris*)
    LIBS="$LIBS -lsocket -lnsl"
;;
hpux*)
    if [ -d /usr/local/lib/hpux32 ]; then
       LDFLAGS="$LDFLAGS -L/usr/local/lib/hpux32"
    fi
;;
esac

# BSD vs GNU date
if date -j >/dev/null 2>&1; then
  pdate() { date -j -f '%Y-%m-%dT%H:%M:%SZ' "$@"; }
else
  pdate() { date -d "$@"; }
fi
RELDATE_PRETTY=$(pdate '_AXEL_RELDATE' '+%B %e, %Y')
AC_SUBST([RELDATE_PRETTY])

# etc path
AC_DEFINE_UNQUOTED([ETCDIR], ["/etc"], ["Define /etc directory path"])

#####################

AC_CONFIG_FILES([
	Makefile
	po/Makefile.in
])
AC_OUTPUT
