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
. ./rel.common
set_abs_path

cd "$CDIR"
set_svn_local
check_local 1
ask_version
test_tag_co
make_tarball
cp -a $CODIR $CODIR.sav
test_tarball
# so Unity is a pain...
build_deb "--enable-appindicator=yes"
mv $CODIR $CODIR.appind
mv $CODIR.sav $CODIR
test_tarball
# for everyone else
build_deb "--enable-appindicator=no"
mv $CODIR $CODIR.appind
echo "output in $CODIR.appind and $CODIR"
show_links
exit 0

