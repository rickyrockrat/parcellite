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
appind_dput='[parcellite-appindicator]
fqdn                    = ppa.launchpad.net
method                  = ftp
incoming                = ~rickyrockrat/ubuntu/parcellite-appindicator/
login                   = anonymous
'
noind_dput='[parcellite]
fqdn                    = ppa.launchpad.net
method                  = ftp
incoming                = ~rickyrockrat/ubuntu/parcellite/
login                   = anonymous
'
set_abs_path
is_dput_app=$(grep -c 'parcellite-appindicator' /etc/dput.cf)
is_dput_noapp=$(grep -c 'parcellite' /etc/dput.cf)
if [ "0" = "$is_dput_app" ] || [ "0" = "$is_dput_noapp" ]; then
	echo "/etc/dput.cf is not set up." 
	echo "May also need devscripts installed"
	echo "put the following entries into dput.cf:"
	echo "$appind_dput"
	echo ""
	echo "$noind_dput"
	exit 1
fi

cd "$CDIR"
set_scm_local
check_local 1
ask_version
test_tag_co
setup_ppa

