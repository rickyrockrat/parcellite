#!/usr/bin/env python
# Copyright (C) 2007-2008 by Xyhthyx <xyhthyx@gmail.com>

# This file is part of Parcellite.

# Parcellite is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# Parcellite is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Run this script as root to uninstall Parcellite v0.4 or less

import os
import sys

# Remove current working directory so python
# doesn't import local parcellite.py file
sys.path.remove(os.getcwd())

try:
	import parcellite
except ImportError:
	print 'Parcellite is not installed.'
	sys.exit(1)

if not os.path.exists('/usr/bin/parcellite'):
	print 'Parcellite is not installed.'
	sys.exit(1)
elif not os.environ['LOGNAME'] == 'root':
	print 'Please run this script as root.'
	sys.exit(1)

files = ['/etc/xdg/autostart/parcellite-startup.desktop', '/usr/share/applications/parcellite.desktop', '/usr/share/parcellite/gui/preferences.glade', '/usr/share/parcellite/gui/clipboard.glade', '/usr/bin/parcellite']

dirs = ['/usr/share/parcellite/gui', '/usr/share/parcellite']

# Remove files
for each in files:
	if os.path.exists(each):
		try:
			os.remove(each)
			print 'Removing-> ' + each
		except:
			pass

# Remove directories
for each in dirs:
	if os.path.exists(each):
		try:
			os.rmdir(each)
			print 'Removing-> ' + each
		except:
			pass

# Remove module
try:
	os.remove(os.path.join(os.path.dirname(parcellite.__file__), 'parcellite.py'))
	print 'Removing-> ' + os.path.join(os.path.dirname(parcellite.__file__), 'parcellite.py')
	os.remove(os.path.join(os.path.dirname(parcellite.__file__), 'parcellite.pyc'))
	print 'Removing-> ' + os.path.join(os.path.dirname(parcellite.__file__), 'parcellite.pyc')
except:
	pass
