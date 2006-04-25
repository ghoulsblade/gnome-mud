dnl Configuration for GNOME-Mud -- a MUD client for the GNOME Desktop
dnl
dnl Copyright (C) 1999,2000,2001,2002,2003,2004,2005 Free Software Foundation, Inc.
dnl
dnl GNOME-Mud is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl GNOME-Mud is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License along
dnl with GNOME-Mud; if not, write to the Free Software Foundation,
dnl Inc. 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
dnl
dnl Process this file with  autoconf to produce a configure script.

AC_PREREQ(2.52)
AC_INIT([GNOME-Mud],[0.10.90], [gnome-mud-list@gnome.org], [gnome-mud])
AC_CONFIG_SRCDIR(src/gnome-mud.c)
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

dnl Release date on LSM format (date +%d%b%y).
LSMDATE="12Feb2004"
AC_SUBST(LSMDATE)

AM_MAINTAINER_MODE

AC_PROG_INTLTOOL([0.23])

GNOME_COMMON_INIT
GNOME_DOC_INIT
GNOME_MAINTAINER_MODE_DEFINES
GNOME_COMPILE_WARNINGS
GNOME_DEBUG_CHECK

ALL_LINGUAS="az ca cs de en_CA en_GB es fr hr mk ml ne nl pa pl pt pt_BR rw sr sr@Latn sv tr uk vi zh_CN"

AM_GLIB_GNU_GETTEXT

GETTEXT_PACKAGE=gnome-mud
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE(GETTEXT_PACKAGE, "gnome-mud", [Gettext domain name])

dnl gconf tests
AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

dnl pkg-config dep checks
GTK_REQUIRED=2.6.0
LIBGLADE_REQUIRED=2.0.1
LIBGNETWORK_REQUIRED=0.0.6
VTE_REQUIRED=0.11.00

PKG_CHECK_MODULES(GMUD, gtk+-2.0 >= $GTK_REQUIRED vte >= $VTE_REQUIRED libglade-2.0 >= $LIBGLADE_REQUIRED libgnetwork-1.0 >= $LIBGNETWORK_REQUIRED)
AC_SUBST(GMUD_CFLAGS)
AC_SUBST(GMUD_LIBS)


dnl Checks for programs.
AC_PROG_CC
AC_ISC_POSIX
dnl AC_SYS_LARGEFILE

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h unistd.h malloc.h termios.h termio.h limits.h getopt.h regex.h)

AC_DEFINE(HAVE_LIBSM, 1, [Define this to have session management.])

dnl Check whether to include mapper support (defaults to no)
AC_ARG_ENABLE(mapper,
	AC_HELP_STRING([--enable-mapper],
		       [Enable automapper support (default=no)]),
		[enable_mapper=$enableval], [enable_mapper=no])

if test "x$enable_mapper" = "xyes" ; then
	AC_DEFINE(ENABLE_MAPPER, 1, [Define if mapper support should be enabled])
fi

dnl Check whether to include MCCP code or not
AC_ARG_ENABLE(mccp,
	AC_HELP_STRING([--enable-mccp],
		       [Enable MCCP (MUD Client Compression Protocol)]),,
		enable_mccp=auto)
if test "x$enable_mccp" != "xno"; then
	AC_CHECK_HEADER(zlib.h, have_zlib=yes, have_zlib=no)
	if test "x$have_zlib" = "xno" -a "x$enable_mccp" = "xyes"; then
	  AC_MSG_ERROR([MCCP support explicitly requested but zlib was not found])
	fi
	if test "x$have_zlib" = "xyes"; then
	  AC_DEFINE(ENABLE_MCCP, 1, [Define if MCCP should be enabled])
	  enable_mccp=yes
	  Z_LIBS="-lz"
	fi
fi
AM_CONDITIONAL(USE_MCCP, test "x$enable_mccp" = xyes)

AC_SUBST(Z_LIBS)

dnl Check whether to include Python support
dnl AC_ARG_WITH(python,
dnl [  --without-python        disable use of python scripting],
dnl         python=$withval, python=python)

dnl Checks for programs.
AC_ISC_POSIX
dnl AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_LN_S
AC_PROG_INSTALL

AC_CHECK_LIB(socket,socket)
AC_CHECK_LIB(nsl,connect)

AC_CHECK_HEADERS(dlfcn.h dl.h)
AC_CHECK_LIB(dl, dlopen, DL_LIB="-ldl",[
	AC_CHECK_LIB(dld, shl_load, DL_LIB="-ldld",[
	AC_CHECK_FUNCS(dlopen, DL_LIB="",
	AC_MSG_ERROR(Dynamic linking is not available on this platform.))])])
AC_SUBST(DL_LIB)  

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT
AC_CHECK_HEADERS(fcntl.h strings.h unistd.h)
AC_CHECK_HEADERS(telnet.h arpa/telnet.h sys/time.h)

AC_HEADER_TIME

dnl Checks for Python
dnl if test "$python" != no; then
dnl 	AC_PATH_PROG(pythonpath, $python)
dnl 	if test "_$pythonpath" = _ ; then
dnl 		python=no
dnl 	else
dnl 		AC_MSG_CHECKING([for Python version >= 2.x])
dnl 		PY_PREFIX=`$pythonpath -c 'import sys ; print sys.prefix'`
dnl 		PY_EXEC_PREFIX=`$pythonpath -c 'import sys ; print sys.exec_prefix'`
dnl 		changequote(<<, >>)dnl
dnl 		PY_VERSION=`$pythonpath -c 'import sys ; print sys.version[0:3]'`
dnl 		PY_MAJOR=`$pythonpath -c 'import sys ; print sys.version[0:2]'`
dnl 		changequote([, ])dnl
dnl 		if test "$PY_MAJOR" = "1."; then
dnl 			python=no
dnl 			AC_MSG_RESULT([no, atlest Python 2.x is needed])
dnl 		else 
dnl 			AC_MSG_RESULT([yes, Python $PY_VERSION])
dnl 			AC_MSG_CHECKING(for Python compile flags)
dnl 			
dnl 			if test -f $PY_PREFIX/include/python$PY_VERSION/Python.h; then
dnl 				PY_LIBS="-lpython$PY_VERSION"
dnl 				PY_LIB_LOC="-L$PY_EXEC_PREFIX/lib/python$PY_VERSION/config"
dnl 				PY_CFLAGS="-I$PY_PREFIX/include/python$PY_VERSION"
dnl 				AC_DEFINE(USE_PYTHON, 1, [Define this to enable python scripting support.])
dnl 				AC_MSG_RESULT([ok])
dnl 				AC_CHECK_LIB(pthread, pthread_create, )
dnl 				dnl needed for freebsd
dnl 				AC_CHECK_LIB(c_r, pthread_create, )
dnl 				dnl needed for python 2.x
dnl 				AC_CHECK_LIB(util, openpty, )
dnl 				AC_CHECK_LIB(db, dbopen, )
dnl 				CFLAGS="$CFLAGS $PY_CFLAGS"
dnl 				CPPFLAGS="$CPPFLAGS $PY_CFLAGS"
dnl 				LIBS="$LIBS $PY_LIB_LOC $PY_LIBS"
dnl 				
dnl 				dnl PyGTK
dnl 				PKG_CHECK_MODULES(PYGTK, pygtk-2.0 >= 1.99.13, [
dnl 					AC_DEFINE(USE_PYGTK, 1, [Define this if you want PyGtk support in Python scripts.])
dnl 					CFLAGS="$CFLAGS $PYGTK_CFLAGS"
dnl 					LIBS="$LIBS $PYGTK_LIBS"
dnl 				])
dnl 			else
dnl 				python=no
dnl 				AC_MSG_RESULT([Can't find Python.h])
dnl 			fi
dnl 		fi
dnl 	fi
dnl fi

dnl python is usually linked against -lm on solaris
dnl if test "$system" = "SunOS"; then
dnl 	if test "$python" != "no"; then
dnl 		if test -z "`echo "$LIBS" | grep '\-lm ' 2>/dev/null`" ; then
dnl 			LIBS="$LIBS -lm"
dnl 		fi
dnl 	fi
dnl fi

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

dnl Checks for library functions.
AC_CHECK_FUNC(bzero)
AC_CHECK_FUNCS(getaddrinfo getnameinfo mkdir socket strdup strerror strstr)

AC_CONFIG_FILES([
  Makefile
  gnome-mud.lsm
  gnome-mud.spec
  doc/Makefile
  doc/gnome-mud-manual/Makefile
  src/Makefile
  src/pixmaps/Makefile
  src/svg/Makefile
  plugins/Makefile
  plugins/python/Makefile
  po/Makefile.in
  ui/Makefile
])
AC_OUTPUT

AC_MSG_NOTICE([GNOME-Mud has been configured with the following options:])

if test "x$enable_mccp" = xyes; then
	AC_MSG_NOTICE([    * MCCP support is enabled])
else
	AC_MSG_NOTICE([    * MCCP support is disabled])
fi

if test "x$enable_mapper" = xyes; then
	AC_MSG_NOTICE([    * Mapper support is enabled])
else
	AC_MSG_NOTICE([    * Mapper support is disabled])
fi

if test -d $srcdir/CVS; then
	AC_MSG_WARN([You're using a CVS version. Expect problems.])
fi
