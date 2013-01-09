#!/bin/sh
# For parcellite deb packaging, install these packages (apt-get):
# dh-make intltool libgtk2.0-dev
if [ $# -lt 1 ]; then
	echo "Usage $0 /path/to/source/tar"
	exit 1
fi
DEBEMAIL=gpib@rickyrockrat.net
DEBFULLNAME="Doug Springer"
export DEBFULLNAME DEBEMAIL
_ARCH=$(uname -m);
case $_ARCH in
  x86_64) ARCH="amd64";;
  i686|i586|i486|i386) ARCH="i386";;
  *) echo "Unknown machine '$_ARCH'. Add machine to this build"
  exit 1;;
esac
dh_make -s -f $1
cd debian
rm emacsen-* manpage.* init.d.ex README.Debian 
cd ..
cp deb/copyright debian
sed "s/ADD_ARCH_HERE/$ARCH/g" deb/control >debian/control
dpkg-buildpackage -rfakeroot

