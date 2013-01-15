# -------------------------------------------------------------------------------
# Autoconf/automake.
# -------------------------------------------------------------------------------
AC_PREREQ([2.5])
AC_INIT([parcellite], m4_esyscmd([./svnversion.sh | tr -d '\n']), [gpib@rickyrockrat.net])
# AC_INIT([parcellite], [lollygag], [gpib@rickyrockrat.com])
AM_INIT_AUTOMAKE([AC_PACKAGE_TARNAME()], [AC_PACKAGE_VERSION()])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR([src/main.c])
AM_CONFIG_HEADER([config.h])

# -------------------------------------------------------------------------------
# Gettext.
# -------------------------------------------------------------------------------

GETTEXT_PACKAGE=parcellite
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext package])

ALL_LINGUAS="ca cs da de es fr hu it ja nb pl pl_PL pt_BR ro ru sv tr zh_CN"
AM_GLIB_GNU_GETTEXT()
AC_PROG_INTLTOOL([0.23])

parcellitelocaledir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(parcellitelocaledir)

AM_GNU_GETTEXT([external])

# -------------------------------------------------------------------------------
# Main settings.
# -------------------------------------------------------------------------------
# prefix=/usr

# -------------------------------------------------------------------------------
# Checks for programs.
# -------------------------------------------------------------------------------
AC_PROG_CC()
AC_PROG_INSTALL()
AC_PROG_MAKE_SET()
AC_CHECK_FUNCS([memset])
PKG_PROG_PKG_CONFIG(0.20)

# -------------------------------------------------------------------------------
# Application indicator.
# -------------------------------------------------------------------------------

APPINDICATOR_REQUIRED_VERSION=0.2.4
APPIND_LIB=appindicator-0.1

AC_ARG_ENABLE([appindicator],
	AS_HELP_STRING([--enable-appindicator[=@<:@no/yes@:>@]],[Build support for application indicators]),
	[enable_appindicator=$enableval],
	[enable_appindicator="yes"])

have_appindicator=no
if test "x$enable_appindicator" = "xyes" ; then
# PKG_CHECK_MODULES completly broken on my system.
  echo -n "checking for $APPIND_LIB... "
  if `$PKG_CONFIG --exists $APPIND_LIB` ; then  
    if `$PKG_CONFIG --atleast-version=$APPINDICATOR_REQUIRED_VERSION $APPIND_LIB` ; then
      have_appindicator=yes
      PKG_CHECK_MODULES([APPINDICATOR], [$APPIND_LIB])
      echo "yes"
    else
      echo "no"
    fi 
  else
    echo "no"
  fi
#  PKG_CHECK_MODULES(APPINDICATOR,[appindicator-0.1>=9.9.9],[have_appindicator=yes],[have_appindicator=no])
fi
#echo "have_appindicator=$have_appindicator"
#echo "pkgcfg=$PKG_CONFIG"
AM_CONDITIONAL(HAVE_APPINDICATOR, test x$enable_appindicator = xyes)

if test "x$have_appindicator" = "xyes" ; then
  AC_DEFINE(HAVE_APPINDICATOR, 1, [ Have appindicator])
  AC_SUBST(APPINDICATOR_CFLAGS)
  AC_SUBST(APPINDICATOR_LIBS)
fi

AC_SUBST(X11_LIBS, -lX11)

# -------------------------------------------------------------------------------
# Checks for libraries.
# -------------------------------------------------------------------------------
# AC_CHECK_LIB(pthread, pthread_create)
pkg_modules="gtk+-2.0 >= 2.10.0"
PKG_CHECK_MODULES([GTK], [$pkg_modules])
#keep pkg-config from being called
GTK_LIBS=`$PKG_CONFIG --libs gtk+-2.0|sed 's/.* \(-lgtk-[[^ ]]*\) .*/\1/'`
gtk_libs=`echo "$GTK_LIBS"|sed 's/-l//'`
#gtk_libs=`$PKG_CONFIG --libs gtk+-2.0|sed 's/.* -l\(gtk-[[^ ]]*\) .*/\1/'`
#echo "gtk_libs = '$gtk_libs'"
AC_CHECK_LIB([$gtk_libs],gtk_status_icon_new,[],AC_MSG_ERROR(Requirement not met: gtk >= 2.10.0))
#AM_PATH_GTK_2_0(2.10.0, [], AC_MSG_ERROR(Requirement not met: gtk >= 2.10.0))
#AM_PATH_GLIB_2_0(2.14.0, [], AC_MSG_ERROR(Requirement not met: glib >= 2.14.0))


#GTK_CFLAGS=`$PKG_CONFIG $pkg_config_args --cflags`
#GTK_LIBS=`$PKG_CONFIG $pkg_config_args --libs|sed 's!.*\(-lgtk-[^ ]*\) .*!\1!'`
# -------------------------------------------------------------------------------
# Checks for header files.
# -------------------------------------------------------------------------------
AC_CHECK_HEADERS([stdlib.h string.h pthread.h libintl.h])

# -------------------------------------------------------------------------------
# Checks for typedefs, structures, and compiler characteristics.
# -------------------------------------------------------------------------------
AC_C_CONST()
AC_C_INLINE()
AC_TYPE_SIZE_T()
AC_PATH_X()

# -------------------------------------------------------------------------------
# Checks for library functions.
# -------------------------------------------------------------------------------
AC_TYPE_SIGNAL()

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 src/Makefile
                 data/Makefile])

AC_OUTPUT([po/Makefile.in])
