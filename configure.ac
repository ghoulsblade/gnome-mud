dnl Process this file with 
dnl aclocal; automake; autoconf to produce a configure script.

AC_INIT
AC_CONFIG_SRCDIR([src/gnome-mud.c])
AC_PREREQ(2.52)
AM_INIT_AUTOMAKE(gnome-mud, 0.9.99.0)
AM_CONFIG_HEADER(config.h:config.h.in)

ALL_LINGUAS="es sv de ca fr"

dnl Checks for programs.
AC_PROG_CC
AC_ISC_POSIX
dnl AC_SYS_LARGEFILE

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h unistd.h malloc.h termios.h termio.h limits.h getopt.h regex.h)

dnl Release date on LSM format (date +%d%b%y).
LSMDATE="02Jul02"
AC_SUBST(LSMDATE)

dnl Pick up the GNOME macros
AM_ACLOCAL_INCLUDE(macros)

AC_DEFINE(HAVE_LIBSM, 1, [Define this to have session management.])

dnl Ability to disable the map feature until I found a fix for it.
AC_ARG_ENABLE(mapper,
	      [  --disable-mapper        do not include automapper part of client],
	      enable_mapper="$enableval",)

if test x$enable_mapper != xyes ; then
  if test x$enable_mapper != x ; then
    AC_DEFINE(WITHOUT_MAPPER, 1, [Define this if you don't want mapper support.])
  fi
fi

dnl Debugging options, internal and GTk+ debugging

AC_ARG_ENABLE(debug, [  --enable-debug          turn on debugging [default=no]], enable_debug="yes", enable_debug="no")

if test "x$enable_debug" = "xyes"; then
  DEBUG_FLAGS="-g -DDEBUG -DG_ENABLE_DEBUG"
else
  DEBUG_FLAGS=""
fi
AC_SUBST(DEBUG_FLAGS)

dnl Check whether user wants simple warnings or advanced warnings
AC_ARG_ENABLE(warnings, [  --enable-warnings       enable more compilation warning checks [default=no]], enable_warnings="yes", enable_warnings="no")

if test "x$enable_warnings" = "xyes"; then
  CFLAGS="$CFLAGS -O -pipe -Wall -Wundef -Wredundant-decls      \
      -Wnested-externs -Wstrict-prototypes -Wmissing-prototypes \
      -Wpointer-arith -Winline -Wcast-qual -ansi"
else
  CFLAGS="$CFLAGS -O -pipe -Wall"
fi
AC_SUBST(CFLAGS)

dnl Check whether to include MCCP code or not
AC_ARG_ENABLE(mccp, [  --enable-mccp           enable MCCP (Mud Client Compression Protocol)],
	      enable_mccp="yes", enable_mccp="no")

dnl Check whether to include Python support
AC_ARG_WITH(python,
[  --without-python        disable use of python scripting],
        python=$withval, python=python)

dnl Init GNOME
GNOME_INIT
dnl Checks for programs.
AC_ISC_POSIX
dnl AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_LN_S
AC_PROG_INSTALL

dnl Checks for libraries.
dnl Replace `main' with a function in -libs:
dnl AM_PATH_GTK(1.2.0,
dnl 	    [LIBS="$LIBS $GTK_LIBS" 
dnl 	     CFLAGS="$CFLAGS $GTK_CFLAGS"],
dnl 	     AC_MSG_ERROR(Cannot find GTK: Is gtk-config in path?))
GNOME_COMPILE_WARNINGS
GNOME_X_CHECKS
AC_CHECK_LIB(socket,socket)
AC_CHECK_LIB(nsl,connect)
AC_CHECK_LIB(z,inflate,[have_zlib="yes" LIBS="$LIBS -lz"])

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
				AC_CHECK_HEADER(pygtk/pygtk.h, AC_DEFINE(USE_PYGTK, 1, [Define this if you want PyGtk support in Python scripts.]))
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
AC_CHECK_FUNCS(mkdir socket strdup strerror strstr)

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

if test "x$have_zlib" = "xyes" && test "x$enable_mccp" = "xyes"; then
  AC_DEFINE(ENABLE_MCCP, 1, [Define this to enable MCCP (Mud Client Compression Protocol) support.])
fi

if test "x$enable_warnings" = "xyes"; then
  CFLAGS="$CFLAGS -O -pipe -Wall -Wundef -Wredundant-decls      \
      -Wnested-externs -Wstrict-prototypes -Wmissing-prototypes \
      -Wpointer-arith -Winline -Wcast-qual -ansi"
else
  CFLAGS="$CFLAGS -O -pipe -Wall"
fi

if test "x$have_zlib" = "xyes" && test "x$enable_mccp" = "xyes"; then
  echo 
  echo "Great! You have Zlib. Will enable MCCP (Mud Client Compression Protocol)"
  echo
fi

AM_GNU_GETTEXT([external])

AC_OUTPUT([
  Makefile
  gnome-mud.lsm
  doc/Makefile
  doc/gnome-mud-manual/Makefile
  doc/gnome-mud-manual/C/Makefile
  doc/gnome-mud-plugin-api/Makefile
  doc/gnome-mud-plugin-api/C/Makefile
  macros/Makefile
  src/Makefile
  plugins/Makefile
  po/Makefile.in
  ])
