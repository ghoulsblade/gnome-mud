dnl Process this file with 
dnl aclocal; automake; autoconf to produce a configure script.


AC_INIT([gnome-mud],[0.10.1],[amcl-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR(src/gnome-mud.c)
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE([AC_PACKAGE_NAME], [AC_PACKAGE_VERSION])
AM_CONFIG_HEADER(config.h)

AC_PREREQ(2.52)

dnl Release date on LSM format (date +%d%b%y).
LSMDATE="10Jun03"
AC_SUBST(LSMDATE)

AM_PATH_GTK_2_0(2.0.0,,exit 1)

AC_DEFINE(GETTEXT_PACKAGE, "gnome-mud", [Gettext domain name])
GETTEXT_PACKAGE=gnome-mud
AC_SUBST(GETTEXT_PACKAGE)
AC_PROG_INTLTOOL([0.23])
ALL_LINGUAS="ca de es fr mk sv tr"
AM_GNU_GETTEXT_VERSION(0.11.5)
AM_GLIB_GNU_GETTEXT

dnl gconf tests
AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

dnl pkg-config dep checks
LIBGNOME_REQUIRED=2.0.0
LIBGNOMEUI_REQUIRED=2.0.0
VTE_REQUIRED=0.10.26

dnl libgnome, libgnomeui needed for all utils
PKG_CHECK_MODULES(GMUD, libgnome-2.0 >= $LIBGNOME_REQUIRED
						libgnomeui-2.0 >= $LIBGNOMEUI_REQUIRED
						vte >= $VTE_REQUIRED)
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

dnl Ability to disable the map feature until I found a fix for it.
AC_ARG_ENABLE(mapper,
[  --disable-mapper        disable mapping support (default=no)],
)

HAVE_MAPPER="yes"
if test "x$enable_mapper" = "xno" ; then
	AC_DEFINE(WITHOUT_MAPPER, 1, [Define this if you don't want mapper support.])
	HAVE_MAPPER="no"
fi

dnl Check whether to include MCCP code or not
AC_ARG_ENABLE(mccp,
[  --enable-mccp           enable MCCP (Mud Client Compression Protocol)],
)

HAVE_ZLIB=no
Z_LIBS=""
if test "x$enable_mccp" = "xyes"; then
  dnl Check for working -lz
  AC_CHECK_HEADER(zlib.h, HAVE_ZLIB=yes, HAVE_ZLIB=no)
    if test "x$HAVE_ZLIB" = "xyes"; then
      Z_LIBS="-lz"
    fi
fi

dnl Check whether to include Python support
AC_ARG_WITH(python,
[  --without-python        disable use of python scripting],
        python=$withval, python=python)

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
AC_CHECK_HEADERS(telnet.h arpa/telnet.h)

dnl Checks for Python
if test "$python" != no; then
	AC_PATH_PROG(pythonpath, $python)
	if test "_$pythonpath" = _ ; then
		python=no
	else
		AC_MSG_CHECKING([for Python version >= 2.x])
		PY_PREFIX=`$pythonpath -c 'import sys ; print sys.prefix'`
		PY_EXEC_PREFIX=`$pythonpath -c 'import sys ; print sys.exec_prefix'`
		changequote(<<, >>)dnl
		PY_VERSION=`$pythonpath -c 'import sys ; print sys.version[0:3]'`
		PY_MAJOR=`$pythonpath -c 'import sys ; print sys.version[0:2]'`
		changequote([, ])dnl
		if test "$PY_MAJOR" = "1."; then
			python=no
			AC_MSG_RESULT([no, atlest Python 2.x is needed])
		else 
			AC_MSG_RESULT([yes, Python $PY_VERSION])
			AC_MSG_CHECKING(for Python compile flags)
			
			if test -f $PY_PREFIX/include/python$PY_VERSION/Python.h; then
				PY_LIBS="-lpython$PY_VERSION"
				PY_LIB_LOC="-L$PY_EXEC_PREFIX/lib/python$PY_VERSION/config"
				PY_CFLAGS="-I$PY_PREFIX/include/python$PY_VERSION"
				AC_DEFINE(USE_PYTHON, 1, [Define this to enable python scripting support.])
				AC_MSG_RESULT([ok])
				AC_CHECK_LIB(pthread, pthread_create, )
				dnl needed for freebsd
				AC_CHECK_LIB(c_r, pthread_create, )
				dnl needed for python 2.x
				AC_CHECK_LIB(util, openpty, )
				AC_CHECK_LIB(db, dbopen, )
				CFLAGS="$CFLAGS $PY_CFLAGS"
				CPPFLAGS="$CPPFLAGS $PY_CFLAGS"
				LIBS="$LIBS $PY_LIB_LOC $PY_LIBS"
				
				dnl PyGTK
				PKG_CHECK_MODULES(PYGTK, pygtk-2.0 >= 1.99.13, [
					AC_DEFINE(USE_PYGTK, 1, [Define this if you want PyGtk support in Python scripts.])
					CFLAGS="$CFLAGS $PYGTK_CFLAGS"
					LIBS="$LIBS $PYGTK_LIBS"
				])
			else
				python=no
				AC_MSG_RESULT([Can't find Python.h])
			fi
		fi
	fi
fi

dnl python is usually linked against -lm on solaris
if test "$system" = "SunOS"; then
	if test "$python" != "no"; then
		if test -z "`echo "$LIBS" | grep '\-lm ' 2>/dev/null`" ; then
			LIBS="$LIBS -lm"
		fi
	fi
fi

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

dnl Checks for library functions.
AC_CHECK_FUNC(bzero)
AC_CHECK_FUNCS(getaddrinfo getnameinfo mkdir socket strdup strerror strstr)

dnl Make sure we link against a working zlib

if test "x$HAVE_ZLIB" = xyes; then
	x11_old_libz=no 
	if test -n "$X_LIBS"; then
	AC_MSG_CHECKING(for libz.a in X11)
		for d in `echo $X_LIBS | sed -e 's/-L//'`; do
			if test -f $d/libz.a; then
				x11_old_libz=$d
				break
			fi
		done
	fi
	if test "x$x11_old_libz" != xno; then
		AC_MSG_RESULT(yes)
		AC_MSG_WARN(there is an old libz.a in $x11_old_libz)
		AC_MSG_WARN($PROGRAM is unable to support compression)
		AC_MSG_WARN(due to library search order issues)
		AC_MSG_WARN(if you wish to have compression support)
		AC_MSG_WARN(please delete $x11_old_libz/libz.a)
		ac_cv_header_zlib_h=no
		HAVE_ZLIB="no, due to conflicting $x11_old_libz/libz.a"
		Z_LIBS=""
	else
		if test -n "$X_LIBS"; then
			AC_MSG_RESULT(no)
		fi
      		AC_DEFINE(ENABLE_MCCP, 1, [Define this to enable MCCP (Mud Client Compression Protocol) support.])
	fi
fi
AC_SUBST(Z_LIBS)

AC_OUTPUT([
  Makefile
  gnome-mud.lsm
  doc/Makefile
  doc/gnome-mud-manual/Makefile
  doc/gnome-mud-manual/C/Makefile
  doc/gnome-mud-plugin-api/Makefile
  doc/gnome-mud-plugin-api/C/Makefile
  doc/omf-install/Makefile
  src/Makefile
  plugins/Makefile
  po/Makefile.in
])

AC_MSG_RESULT([
	Gnome-mud has been configured successfully.

	MCCP support	: $HAVE_ZLIB
	Mapper support	: $HAVE_MAPPER
	Plugin support	: python=$python
dnl	Compiler version: $CCVER
	Compiler flags	: $CFLAGS
])

if test -d $srcdir/CVS; then
	echo "WARNING: You're using a CVS version. Expect problems."
	echo
fi
