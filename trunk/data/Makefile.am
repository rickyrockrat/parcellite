## Process this file with automake to produce Makefile.in
##
##    Copyright (C) 2010 Doug Springer (gpib at rickyrockrat dot net)
##
##  This file is part of parcellite.
##  Parcellite is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 3 of the License, or
##  (at your option) any later version.
## 
##  Parcellite is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
## 
##  You should have received a copy of the GNU General Public License
##  along with this program.  If not, see <http://www.gnu.org/licenses/>.

@INTLTOOL_DESKTOP_RULE@

xdgdatadir=@XDGDATADIR@
app_name=parcellite

app_icon_files = \
	$(app_name:%=%.png) \
	$(app_name:%=%.svg) \
	$(app_name:%=%.xpm) 

desktopdir = $(datadir)/applications
desktop_in_file = $(app_name:%=%.desktop.in)
desktop_DATA = $(app_name:%=%.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_file = $(app_name:%=%-startup.desktop.in)
autostart_DATA = $(app_name:%=%-startup.desktop)

pixmapdir = $(datadir)/pixmaps

pixmap_DATA = $(app_icon_files)


EXTRA_DIST = \
  $(desktop_in_file) \
  $(autostart_in_file) \
  $(app_icon_files) 

CLEANFILES = \
  $(desktop_DATA) \
  $(autostart_DATA)

DISTCLEANFILES = \
  $(desktop_DATA) \
  $(autostart_DATA) 
