dnl
AC_DEFUN([KORVA_WARNING_FLAGS],
[
    KORVA_WARNING_CFLAGS="-Wall -Wextra"
    _FOUND_FLAGS=""
    m4_foreach(flag, [no-unused-parameter, missing-declarations, shadow,
                      redundant-decls, no-missing-field-initializers,
                      missing-noreturn, pointer-arith, write-strings,
                      inline, format-nonliteral, cast-align,
                      format-security, switch-enum, switch-default],
               [
                 AS_COMPILER_FLAG(-W[]flag,
                                  [
                                     _FOUND_FLAGS="$[]_FOUND_FLAGS -W[]flag"
                                  ], [])
               ])
    KORVA_WARNING_CFLAGS="-Wall -Wextra $[]_FOUND_FLAGS"

    AC_SUBST([KORVA_WARNING_CFLAGS])
])
