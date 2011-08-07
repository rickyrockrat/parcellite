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
