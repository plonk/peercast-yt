#!/bin/sh

# Peercast install script for linux
# By Romain Beauxis <toots@rastageeks.org>

# Arguments (optionnals) : install.sh <prefix path for install> <prefix path for KDE protocol file>

PREFIX=$1
KDEPREFIX=$2

if [ "$PREFIX" = "" ]; then
	PREFIX="/usr/local"
fi 

if [ "$KDEPREFIX" = "" ]; then
	if [ -f `which kde-config` ]; then
		KDEPREFIX="`kde-config --prefix`"
	else
	KDEPREFIX="/usr"
	fi
fi

insdir=$PREFIX/sbin
sharedir=$PREFIX/share/peercast
libdir=$PREFIX/lib
libsoversion=1.0
peercastlibdir=$libdir/peercast
kdedir=$KDEPREFIX/share/services
includedir=$PREFIX/include/peercast

echo "Doing the install.."


echo "- Peercast bin and file."
mkdir -p $sharedir $insdir $peercastlibdir $kdedir $includedir
cp -f ./peercast $insdir
cp -f ./libpeercast.so.$libsoversion ./libpeercast.a $libdir
/usr/bin/strip --strip-unneeded $libdir/libpeercast.so.$libsoversion
echo "Please, make sure that the directory for shared library is listed in known by ldconfig"
echo "Else, use this command: export LD_LIBRARY_PATH=$libdir:\$LD_LIBRARY_PATH"
echo "Before launching peercast."
/sbin/ldconfig -n $libdir
	# Library headers:
ln -s libpeercast.so.$libsoversion $libdir/libpeercast.so
cp ../../core/unix/*.h ../../core/common/*.h $includedir 
cp -rf ./../html $sharedir
chmod +x $insdir/peercast

echo "- installing peercast protocol handler."
echo "-> peercast:// handling scripts"
cp -rf ./scripts $peercastlibdir
echo "* Setting correct prefix for scripts"
rm -f $libdir/scripts/peercast_khandler.sh
cat ./scripts/peercast_khandler.sh | sed -e s#/usr/local#$PREFIX# > $peercastlibdir/scripts/peercast_khandler.sh
chmod +x $peercastlibdir/scripts/*
echo "-> Protocol handler for KDE"
echo "* Setting correct prefix for scripts"
cat ./kde/peercast.protocol | sed -e s#/usr/local#$PREFIX# > $kdedir/peercast.protocol
echo "-> Protocol handler for Gnome"
echo "TODO: Check it on PeerCast Wiki.. I don't have gnome!"

