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
# 
# This script should be run from an uptodate SVN checkout of trunk
# Run from trunk, it creates directories in the parent.
#
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
. ./rel.common

#BEGIN Script
set_abs_path

cd "$CDIR"
set_svn_local
check_local
check_translations
svn_cp_to_tag
update_pot
#tag="1.0.2rc7Beta"
#turl="$BURL/tags/$tag"
#CODIR="$ASVN/test.$tag"
rel_deb
remote_i386
copy_files_to_sourceforge

