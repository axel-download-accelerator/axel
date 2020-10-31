# TODO: we need a true replacement for AC_INIT...

# _AXEL_ntsinc(FILE) - sinclude not to be tracked by automake
# ---------------------------------------------------------------------
m4_define([_AXEL_ntsinc], defn([m4_sinclude]))

# _AXEL_READL(FILE, HARD-DEP?)
# ---------------------------------------------------------------------
m4_define([_AXEL_READL],
[m4_bpatsubst(m4_quote(m4_if([$2],,
  [_AXEL_ntsinc([$1])],
  [m4_apply([m4_include], [[$1]])])), [ *
])])

# _AXEL_GIT_HEAD
# ---------------------------------------------------------------------
m4_define([_AXEL_GIT_HEAD],
[m4_define([__HEAD], m4_quote(_AXEL_READL([.git/HEAD])))dnl
m4_if(m4_substr(m4_quote(__HEAD), 0, 4), [ref:],
  [m4_quote(_AXEL_READL([.git/]m4_bpatsubst(__HEAD, [^ref: *])))],
  [m4_quote(__HEAD)])[]dnl
m4_undefine([__HEAD])dnl
])

# _AXEL_GIT_TAG(TAG)
# ---------------------------------------------------------------------
m4_define([_AXEL_GIT_TAG],
[m4_quote(_AXEL_READL([.git/refs/tags/$1]))])

# AXEL_VER_READ(FILE)
# ---------------------------------------------------------------------
AC_DEFUN_ONCE([AXEL_VER_READ],
[_AC_INIT_LITERAL([$1])
m4_define([_AXEL_RELSTR],
  m4_quote(_AXEL_READL([$1], 1)))
m4_define([_AXEL_RELDATE],
  m4_quote(
    m4_bpatsubst(
      m4_quote(
        m4_bpatsubst(
          m4_quote(_AXEL_RELSTR),
          [[^ ]* ])), [ .*])))
m4_define([_AXEL_VERSION],
  m4_quote(m4_bpatsubst(m4_quote(_AXEL_RELSTR), [ .*])))
m4_define([_AXEL_COMMIT], m4_quote(_AXEL_GIT_HEAD))
m4_define([_AXEL_VERSUF],
  [m4_if(m4_quote(_AXEL_COMMIT),,,
    [m4_if(m4_quote(_AXEL_COMMIT), m4_quote(_AXEL_GIT_TAG([v]_AXEL_VERSION)),,
      [[+g]m4_quote(m4_substr(_AXEL_COMMIT, 0, 6))])])])
m4_define([AC_PACKAGE_STRING], m4_quote(AC_PACKAGE_NAME AC_PACKAGE_VERSION))
m4_define([AC_PACKAGE_VERSION], m4_quote(_AXEL_VERSION[]_AXEL_VERSUF))
])

# AXEL_PKG(PACKAGE-NAME, BUG-REPORT, [TAR-NAME])
# ---------------------------------------------------------------------
AC_DEFUN_ONCE([AXEL_PKG],
[_AC_INIT_LITERAL([$1])
_AC_INIT_LITERAL([$2])
AC_BEFORE([AXEL_VER_READ])
m4_define([AC_PACKAGE_NAME], [$1])
m4_define([AC_PACKAGE_TARNAME],
  m4_default([$3], [m4_bpatsubst(m4_tolower([$1]),
				 [[^_abcdefghijklmnopqrstuvwxyz0123456789]],
				 [-])]))
m4_define([AC_PACKAGE_BUGREPORT], [$2])
])


# MKINSTALLDIRS isn't needed; remove
AC_DEFUN([AM_MKINSTALLDIRS], [])

# AXEL_CHECK_MACRO(MACRO, HEADER, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ---------------------------------------------------------------------
AC_DEFUN([AXEL_CHECK_MACRO],
[_AC_INIT_LITERAL([$1])
_AC_INIT_LITERAL([$2])
AC_CACHE_CHECK([for $1], [axel_cv_macro_$1],
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <]$2[>]], [[(void)]$1;])],
      [axel_cv_macro_$1=yes],
      [axel_cv_macro_$1=no])
  ])
AS_IF([test "x$axel_cv_macro_$1" = "xyes"],
  [$3], m4_default([$4], [
    AC_MSG_ERROR([The $1 macro is required; it should be defined by $2.])
  ]))
])

# AXEL_DEFAULT_TYPE(TYPE-ID, DEFAULT)
# ---------------------------------------------------------------------
AC_DEFUN([AXEL_DEFAULT_TYPE], [
  _AC_INIT_LITERAL([$1])
  AH_VERBATIM(AS_TR_CPP([HAVE_$1_default]),
    [#ifndef ]AS_TR_CPP([HAVE_$1])[
typedef $2 $1;
#endif])
])

# AXEL_CHECK_TYPE(TYPE-ID, COMPAT-TEST, [INCLUDES])
# ---------------------------------------------------------------------
AC_DEFUN([AXEL_CHECK_TYPE], [
  _AC_INIT_LITERAL([$1])
  AC_CHECK_TYPE([$1], [
    AC_CACHE_CHECK([for $1 compatibility], [axel_cv_type_compat_$1], [
      AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT([$3])],
	  [switch(0) case 0: case ($2):;])],
	[axel_cv_type_compat_$1=yes], [axel_cv_type_compat_$1=no])
    ])
    AS_IF([test "x$axel_cv_type_compat_$1" = xno],
      [AC_MSG_ERROR([The $1 type is not fit ($2).])])
    AC_DEFINE(AS_TR_CPP([HAVE_$1]), [1],
              [Define to 1 if the system has the type `$1'.])
  ],, [AC_INCLUDES_DEFAULT([$3])])
])
