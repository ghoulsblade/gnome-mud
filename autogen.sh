#!/bin/sh
# Run this to generate all the initial makefiles, etc.
# This was lifted from the Gimp, and adapted slightly by
# Raph Levien and Robin Ericsson.

DIE=0

PROG="GNOME-Mud"
        
(autoconf --version) < /dev/null > /dev/null 2>&1 || {
        echo 
        echo "You must have autoconf installed to compile $PROG."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have automake installed to compile $PROG."
        echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.3.tar.gz"
        echo "(or a newer version if it is available)"
        DIE=1
}
if test "$DIE" -eq 1; then
        exit 1
fi

if test -z "$*"; then
        echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi
        
for dir in .   
do
  echo processing $dir
  (cd $dir; \
  aclocalinclude="$ACLOCAL_FLAGS"; \
  aclocal -I macros $aclocalinclude; \
  autoheader; automake --add-missing;
  autoconf)
done

if test x$1 != xno ; then
  ./configure "$@"
fi

echo
echo "readme_doc.h, authors_doc.h and version.h is not created."
echo "cd src; make -f Makefile.mkhelp; make version.h with GNU make"
echo "(that is gmake for *BSD ;)"
echo
echo "Now type 'make' to compile $PROG."

