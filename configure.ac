#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([axel], [2.6], [https://github.com/eribertomota/axel/issues])
AC_CONFIG_SRCDIR([src/conf.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_CHECK_PROGS([GETTEXT], [gettext], "no")
if test "x$GETTEXT" = "xno"; then
    AC_MSG_ERROR([axel requires gettext])
fi

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h libintl.h limits.h locale.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset select setlocale socket strcasecmp strchr strrchr strstr])

# Flags
CFLAGS="$CFLAGS -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall"
LDFLAGS="$LDFLAGS -pthread"

#### Definitions ####

# OS
arch=$(uname -s)
AC_DEFINE_UNQUOTED([ARCH], ["$arch"], ["Define architecture"])

# etc path
AC_DEFINE_UNQUOTED([ETCDIR], ["/etc"], ["Define /etc directory path"])

# Other
AH_TEMPLATE([_REENTRANT], [Local definition])
AC_DEFINE_UNQUOTED([_REENTRANT])
AH_TEMPLATE([_THREAD_SAFE], [Local definition])
AC_DEFINE_UNQUOTED([_THREAD_SAFE])

#####################

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
