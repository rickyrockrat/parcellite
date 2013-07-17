#!/bin/sh
# For parcellite deb packaging, install these packages (apt-get):
# dh-make intltool libgtk2.0-dev
. ./rel.common
if [ $# -lt 1 ]; then
	echo "Usage $0 <opts>  configopts"
	echo " -c config opts"
	echo " -k gpg key ID"
	echo " -p /path/to/source/tar"
	exit 1
fi
while  getopts c:k:p: opt ; do
  #echo "opt=$opt arg=$OPTARG"
  case $opt in
    c) OPTS=$OPTARG ;;
    k) KEY="-k$OPTARG" ;;
    p) TARPATH=$OPTARG ;;
    *) echo "Invalid option $OPTARG"
       exit 1
       ;;
  esac
done
select_gpg_keys
DEBEMAIL=$GEMAIL
DEBFULLNAME=$GNAME
export DEBFULLNAME DEBEMAIL
_ARCH=$(uname -m);
case $_ARCH in
  x86_64) ARCH="amd64";;
  i686|i586|i486|i386) ARCH="i386";;
  *) echo "Unknown machine '$_ARCH'. Add machine to this build"
  exit 1;;
esac
dh_make -s -f $KEY $TARPATH
cd debian
rm emacsen-* manpage.* init.d.ex README.Debian 
cd ..
cp deb/copyright debian
sed "s/ADD_ARCH_HERE/$ARCH/g" deb/control >debian/control
if [ -n "$OPTS" ]; then
  echo "DEB_CONFIGURE_EXTRA_FLAGS += $OPTS" >> debian/rules
fi
dpkg-buildpackage -rfakeroot

