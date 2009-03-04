dnl Configuration for GNOME-Mud -- a MUD client for the GNOME Desktop
dnl
dnl Copyright (C) 1998-2005 Robin Ericsson <lobbin@localhost.nu>
dnl Copyright (C) 2005-2009 Les Harris <lharris@gnome.org>
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
AC_INIT([GNOME-Mud],[0.11.2], [gnome-mud-list@gnome.org], [gnome-mud])
AC_CONFIG_SRCDIR(src/gnome-mud.c)
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

PKG_PROG_PKG_CONFIG(0.9.0)

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.35.0])

GNOME_COMMON_INIT
# Docs are disabled for now
#GNOME_DOC_INIT
GNOME_MAINTAINER_MODE_DEFINES
GNOME_COMPILE_WARNINGS
GNOME_DEBUG_CHECK

AM_GLIB_GNU_GETTEXT

GETTEXT_PACKAGE=gnome-mud
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE(GETTEXT_PACKAGE, "gnome-mud", [Gettext domain name])

dnl gconf tests
AM_GCONF_SOURCE_2
AC_PATH_PROG(GCONFTOOL, gconftool-2)

if test x"$GCONFTOOL" = xno; then
	AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
fi

dnl pkg-config dep checks
GMODULE_REQUIRED=2.0.0
GTK_REQUIRED=2.10.0
LIBGLADE_REQUIRED=2.0.1
LIBGNET_REQUIRED=0.22
VTE_REQUIRED=0.11.00
PCRE_REQUIRED=6.0.0
GCONF_REQUIRED=0.20
GSTREAMER_REQUIRED=0.10

PKG_CHECK_MODULES(GMUD, gtk+-2.0 >= $GTK_REQUIRED vte >= $VTE_REQUIRED libglade-2.0 >= $LIBGLADE_REQUIRED libpcre >= $PCRE_REQUIRED gmodule-2.0 >= $GMODULE_REQUIRED gnet-2.0 >= $LIBGNET_REQUIRED gconf-2.0 >= $GCONF_REQUIRED)
AC_SUBST(GMUD_CFLAGS)
AC_SUBST(GMUD_LIBS)

dnl Checks for programs.
AC_PROG_CC
AC_ISC_POSIX

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_DIRENT
AC_CHECK_HEADERS(strings.h)

AC_DEFINE(HAVE_LIBSM, 1, [Define this to have session management.])

dnl Check whether to include MCCP code or not
AC_ARG_ENABLE(mccp,
              AC_HELP_STRING([--enable-mccp],
                             [Enable MCCP (MUD Client Compression Protocol) Default=auto]),,
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

dnl Check whether to include GStreamer code or not
AC_ARG_ENABLE(gstreamer,
	AC_HELP_STRING([--enable-gstreamer],
		       [Enable GStreamer used for MSP (Mud Sound Protocol) Default=auto]),,
		enable_gstreamer=auto)
if test "x$enable_gstreamer" != "xno"; then
	PKG_CHECK_EXISTS(gstreamer-0.10, have_gst=yes, have_gst=no)	
	if test "x$have_gst" = "xno" -a "x$enable_gstreamer" = "xyes"; then
	  AC_MSG_ERROR([Gstreamer support explicitly requested but libgstreamer was not found])
	fi
	if test "x$have_gst" = "xyes"; then
	  AC_DEFINE(ENABLE_GST, 1, [Define if GStreamer should be enabled])
	  enable_gstreamer=yes
	  PKG_CHECK_MODULES(MSP, gstreamer-0.10 >= $GSTREAMER_REQUIRED)
	fi
fi
AM_CONDITIONAL(USE_GSTREAMER, test "x$enable_gstreamer" = xyes)
AC_SUBST(MSP_CFLAGS)
AC_SUBST(MSP_LIBS)

dnl Check whether to build debug logger or not.
AC_ARG_ENABLE(debug-logger,
              AC_HELP_STRING([--enable-debug-logger],
                             [Enable debugging info logger Default=no]),,
                             enable_debug_logger=no)
if test "x$enable_debug_logger" != "xno"; then
    AC_DEFINE(ENABLE_DEBUG_LOGGER, 1, [Define if Debug Logging is desired])
    enable_debug_logger=yes
fi
AM_CONDITIONAL(USE_DEBUG_LOGGER, test "x$enable_debug_logger" = xyes)

dnl Checks for programs.
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_LN_S
AC_PROG_INSTALL

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

AC_CONFIG_FILES([
  Makefile
  gnome-mud.spec
  data/Makefile
  data/icons/Makefile
  doc/Makefile
  src/Makefile
  plugins/Makefile
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

if test "x$enable_gstreamer" = xyes; then
	AC_MSG_NOTICE([    * GStreamer support is enabled])
else
	AC_MSG_NOTICE([    * GStreamer support is disabled])
fi

if test "x$enable_debug_logger" = xyes; then
    AC_MSG_NOTICE([    * Debug logger is enabled])
else
    AC_MSG_NOTICE([    * Debug logger is disabled])
fi

if test -d $srcdir/.svn; then
	AC_MSG_WARN([You're using a svn version. Expect problems.])
fi

if test -d $srcdir/.git; then
	AC_MSG_WARN([You're using a git version. Expect problems.])
fi
