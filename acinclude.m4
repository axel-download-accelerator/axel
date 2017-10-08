# TODO: we need a true replacement for AC_INIT...

# _AXEL_READL(FILE, HARD-DEP?)
# ---------------------------------------------------------------------
m4_define([_AXEL_READL],
[m4_bpatsubst(m4_quote(m4_if([$2],,
  [m4_sinclude([$1])],
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
m4_define([_AXEL_VERSION], m4_quote(m4_bpatsubst(m4_quote(_AXEL_READL([$1],1)), [ .*])))
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
