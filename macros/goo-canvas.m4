 
# AM_PATH_GOOCANVAS([ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]])
AC_DEFUN([AM_PATH_GOOCANVAS],
[
AC_PROVIDE([AM_PATH_GOOCANVAS])

AC_ARG_WITH(goocanvas-prefix, [  --with-goocanvas-prefix=PFX Prefix where GOOCANVAS has been built],
 goocanvas_prefix="$withval",
 goocanvas_prefix="")


saved_LIBS="$LIBS"
saved_CXXFLAGS="$CXXFLAGS"


ac_cv_build_alias=${ac_cv_build_alias:=$build_alias}
if test x$goocanvas_prefix != x; then

 # the majority of cases, we will try to configure with:
 # --with-goocanvas-prefix=/some/thing
 #

  case $ac_cv_build_alias in
  *-mingw*)
    GOOCANVAS_CFLAGS="-I$goocanvas_prefix/include/goocanvas-1.0.0/goocanvas"
    break;;
  *)
    GOOCANVAS_CFLAGS="-I$goocanvas_prefix/include/goocanvas-1.0"
    break;;
  esac
  #
  # Similarly for goocanvas, the uninstalled library position is simply in
  # $goocanvas_prefix, but the installed is in the standard prefixed subdirectory.
  #
  # SGI compiler CC (CXX=CC) needs -lm to link maths library, but 
  # GCC c++ does not.
  #
  GOOCANVAS_LDOPTS="-L$goocanvas_prefix/lib -lgoocanvas"

else
  # the compiler looks in the "standard" places for GOOCANVAS. (or uses pkgconfig)

  case $ac_cv_build_alias in
  *-mingw*)
    # we can use pkg-config, so why not
    if test -z "${PKG_CONFIG}"; then
      GOOCANVAS_CFLAGS="-I/usr/include/goocanvas-1.0.0"
    else
      GOOCANVAS_CFLAGS=`$PKG_CONFIG goocanvas --cflags`
    fi
    break;;
  *)
    GOOCANVAS_CFLAGS="-I/usr/include/goocanvas-1.0"
    break;;
  esac
  if test -z "${PKG_CONFIG}"; then
     GOOCANVAS_LDOPTS="-lgoocanvas"
  else
     GOOCANVAS_LDOPTS=`$PKG_CONFIG goocanvas --libs`
  fi
fi

AC_MSG_CHECKING([for Goocanvas])

LIBS="$saved_LIBS $GOOCANVAS_LDOPTS $GTK_LIBS"
CXXFLAGS="$saved_CXXFLAGS $GOOCANVAS_CFLAGS $GTK_CFLAGS"


dnl # 20130929: why do I do this?  I don't remember.  Using libtool 
dnl # in this way is not a good idea (configure makes libtool).
dnl #
dnl #
dnl # AC_TRY_LINK uses the c compiler (set by AC_LANG), so we will
dnl # temporarily reassign $CC to the c++ compiler.
dnl #
dnl AC_LANG_PUSH(C++)
dnl save_CXX="$CXX"
dnl case $ac_cv_build_alias in
dnl   *-mingw*)
dnl     # only do the libtool for non-mingw, probably should be libtool version
dnl     # dependend!?
dnl     break;;
dnl   *)
dnl     # note that we use ./libtool (running in the build dir) because $LIBTOOL is wrong(!)
dnl     CXX="libtool --mode=link $CXX"
dnl     break;;
dnl esac


AC_LANG_PUSH(C++)
AC_TRY_LINK([#include "goocanvas.h"] ,[ GooCanvas *a;  ], have_goocanvas=yes, have_goocanvas=no)
AC_LANG_POP

AC_MSG_RESULT($have_goocanvas)

if test x$have_goocanvas = xyes; then

 GOOCANVAS_CFLAGS="$GOOCANVAS_CFLAGS"
 GOOCANVAS_LIBS="$GOOCANVAS_LDOPTS"

ifelse([$1], , :, [$1])

else

 # we want to fail if a prefix was given but goocanvas was not found
 # have_goocanvas=no here
 # 
 if test x$goocanvas_prefix != x ; then
	# fail
	AC_MSG_ERROR([--with-goocanvas-prefix was specified, but no goocanvas was found])
 fi

 GOOCANVAS_LIBS=""
 GOOCANVAS_CFLAGS=""
 ifelse([$2], , :, [$2])

fi

# restore
#
LIBS="$saved_LIBS"
CXXFLAGS="$saved_CXXFLAGS"


AC_SUBST(GOOCANVAS_CFLAGS)
AC_SUBST(GOOCANVAS_LIBS)

])
