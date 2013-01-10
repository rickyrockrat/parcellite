#!/bin/bash
# Copyright (C) 2011-2013 by rickyrockrat <gpib at rickyrockrat dot net>
#
# This file is part of Parcellite.
#
# Parcellite is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Parcellite is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# Notes:
# Do a svn cp:
#  svn cp  -m"prep for 1.0.2rc3"  https://parcellite.svn.sourceforge.net/svnroot/parcellite/trunk  https://parcellite.svn.sourceforge.net/svnroot/parcellite/tags/1.0.2rc3
# Do a svn co:
#  svn co https://parcellite.svn.sourceforge.net/svnroot/parcellite/tags/1.0.2rc3
# Remove svnversion.sh
# Change the configure.ac to the new version:
# AC_INIT([parcellite], [1.0.2rc2], [gpib@rickyrockrat.com])
# Update ChangeLog
# Run the autogen:
# gettextize
# intltoolize
# aclocal
# autoheader
# automake
# autoconf
# Create the tar ball:
#  tar --exclude .svn -cjf 1.0.2rc3.tar.bz2 1.0.2rc3
#  mkdir build
#  cd build
#  tar -xjf ../1.0.2rc3.tar.bz2
#  mv 1.0.2rc3 parcellite-1.0.2rc3
#  tar -czf parcellite-1.0.2rc3.tar.gz parcellite-1.0.2rc3
# Test tarball:
#  cd parcellite-1.0.2rc3
#  ./configure
#  ./make
#  src/parcellite &
# Post the news to http://parcellite.sourceforge.net/
# Edit the existing, change display to HTML, then create new post, set it to HTML, then copy 
# the links:
# The source can be <a href="http://sourceforge.net/projects/parcellite/files/parcellite/parcellite-1.0.2rc3/parcellite-1.0.2rc3.tar.gz/download">downloaded here</a>.
# 
# Here are the debian binaries (.deb) files:
# <a href="http://sourceforge.net/projects/parcellite/files/parcellite/parcellite-1.0.2rc3/parcellite_1.0.2rc3-1_i386.deb/download">i386</a>
# <a href="http://sourceforge.net/projects/parcellite/files/parcellite/parcellite-1.0.2rc3/parcellite_1.0.2rc3-1_amd64.deb/download">amd64</a>.
# 
# Then dump the top of the changelog
# 
# Then build the debs:
# sudo apt-get install dh-make 
# tar -xzf parcellite-1.0.2rc3.tar.gz
# cd parcellite-1.0.2rc3
# ./build-debpkg.sh ../parcellite-1.0.2rc3.tar.gz

PURL="https://parcellite.svn.sourceforge.net"
BURL="$PURL/svnroot/parcellite"

# description
check_err () {
  if [ $? -ne 0 ]; then
		echo "$1 Failed. Abort"
		exit 1
	fi
}
# Make sure we are checked in.
check_local () {
  echo "$FUNCNAME"
  M=$(svn st|grep -c "^M")
  if [ "$M" != "0" ]; then
    echo "local is not up to date"
    exit 1
  fi
}
# Copy the trunk to tag
svn_cp_to_tag () {
  echo "$FUNCNAME"
  echo "Tag to copy trunk to?"
  read tag
  turl="$BURL/tags/$tag"
  E=$(svn ls $turl --depth empty 2>&1|grep -c "non-exist")
  if [ "$E" = "0" ]; then
    echo "$turl exists."
    exit 1
  fi
  CODIR="$ASVN/test.$tag"
  if [ -e "$CODIR" ]; then
    echo "$CODIR exists. Refusing to overwrite."
    exit 1
  fi
  echo "Description for tag?"
  read desc
  echo "Contacting $PURL for svn cp"
  svn cp -m"$desc" $BURL/trunk $turl
  check_err "svn copy"
}
#Check to be sure we have the correct directory orientation
set_svn_local (){
  echo "$FUNCNAME"
  S=$(grep -c "$BURL" "$CDIR/.svn/entries")
  if [ "$S" = "0" ]; then	
    echo "$CDIR does not appear to be a working local copy"
	  exit 1
  fi
 ASVN=$(dirname "$CDIR")
 if [ -e "$ASVN/.svn" ]; then
   echo "directory '$ASVN should NOT be under svn"
   exit 1
 fi
}
# make check out dir and and check out tag
test_tag_co () {
  echo "$FUNCNAME"
  mkdir -p "$CODIR"
  check_err "mkdir co"
  cd "$CODIR"
  echo "performing svn co $turl"
  svn co "$turl"
  check_err "svn co"
  cd $tag
  check_err "cd $tag"
  rm svnversion.sh
  check_err "rm svnver"
  sed -i "s/\(AC_INIT.*\)m4_.*)\(.*rickyrockrat.*\)/\1[$tag]\2/" configure.ac
  check_err "update configure.ac"
  ./autogen.sh
}
make_tarball () {
  echo "$FUNCNAME"
  cd "$CODIR"
  tdir="parcellite-$tag"
  mv $tag $tdir
  check_err "mv svn co dir"
  tar --exclude .svn -czf $tdir.tar.gz $tdir
  check_err "tar -c"
}
test_tarball() {
  echo "$FUNCNAME"
  cd "$CODIR"
  check_err "cd dir"
  mkdir tar_test
  check_err "mkdir tar_test"
  cd tar_test
  tar -xzf ../$tdir.tar.gz
  check_err "untar"
  cd "$tdir"
  check_err "cd tardir"
  ./configure
  check_err "config"
  make
  check_err "make"
}
build_deb () {
  echo "$FUNCNAME"
  cd "$CODIR/tar_test/$tdir"
  check_err "cd tar_test" 
  ./build-debpkg.sh ../../$tdir.tar.gz
  check_err "build deb"
  cd ..
  mv *.deb ..
}
#
set_abs_path () {
  CDIR=$(dirname $0)
  T=$(dirname $CDIR)
  if [ "$T" = "$CDIR" ]; then # ./ invocation
    CDIR=$(pwd)
  fi
  echo "Using '$CDIR'"
}
show_links () {
echo "Source Forge Links"
echo "The source can be <a href=\"http://sourceforge.net/projects/parcellite/files/parcellite/parcellite-$tag/parcellite-$tag.tar.gz/download\">downloaded here</a>."
echo "<a href=\"http://sourceforge.net/projects/parcellite/files/parcellite/parcellite-$tag/parcellite_$tag-1_i386.deb/download\">i386</a>"
echo "<a href=\"http://sourceforge.net/projects/parcellite/files/parcellite/parcellite-$tag/parcellite_$tag-1_amd64.deb/download\">amd64</a>."
}
#BEGIN Script
set_abs_path

cd "$CDIR"
set_svn_local
check_local
#svn_cp_to_tag
tag="1.0.2rc7Beta"
turl="$BURL/tags/$tag"
CODIR="$ASVN/test.$tag"
test_tag_co
make_tarball
test_tarball
build_deb
echo "output in $CODIR"
exit 0


	 


