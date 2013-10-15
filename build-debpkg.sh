#!/bin/sh
# For parcellite deb packaging, install these packages (apt-get):
# dh-make intltool libgtk2.0-dev
INCD=$(dirname $0)
. "$INCD/rel.common"
if [ $# -lt 1 ]; then
	echo "Usage $0 <opts>  configopts"
	echo " -c config opts"
	echo " -k gpg key ID"
	echo " -p /path/to/source/tar"
	echo " -t tag tag (like 1.1.4) we are operating on."
	exit 1
fi
while  getopts c:k:p:t: opt ; do
  #echo "opt=$opt arg=$OPTARG"
  case $opt in
    c) OPTS="$OPTARG" ;;
    k) KEY="-k$OPTARG" ;;
    p) TARPATH=$OPTARG ;;
    t) tag="$OPTARG" 
    ask_version $tag ;;
    *) echo "Invalid option $OPTARG"
       exit 1
       ;;
  esac
done
log "OPTS='$OPTS'"
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
dh_make -s $KEY  -f $TARPATH
log "build-debpkg.sh: currently in $(pwd)"
cd debian
rm emacsen-* manpage.* init.d.ex README.Debian 
cd ..
cp deb/copyright debian

sed "s/ADD_ARCH_HERE/$ARCH/g" deb/control >debian/control
cp ../rules debian/rules
#if [ -n "$OPTS" ]; then
#  echo "DEB_CONFIGURE_EXTRA_FLAGS += $OPTS" >> debian/rules
#fi
dpkg-buildpackage -rfakeroot

