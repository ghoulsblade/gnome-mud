#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="gnome-mud"

(test -f $srcdir/configure.ac) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

if [ -f configure.in ]; then
    echo "configure.in is not a symlink to configure.ac."
    echo "Move it out of the way and try again."
elif [ ! -L configure.in ]; then
    rm -f configure.in
    ln -s configure.ac configure.in
fi

which gnome-autogen.sh || {
	echo "You need to install gnome-common from the GNOME CVS"
	exit 1
}

USE_GNOME2_MACROS=1 . gnome-autogen.sh

rm -f configure.in
