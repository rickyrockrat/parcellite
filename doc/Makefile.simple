# Copyright (C) 2010-2022 Doug Springer <gpib@rickyrockrat.net>
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

MANDIR:=$(INSTDIR)/share/man/man1

top_srcdir = ..

srcdir:=.

# po dir
INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
GMSGFMT = /usr/bin/msgfmt
MSGFMT = /usr/bin/msgfmt
XGETTEXT = /usr/bin/xgettext

all: 


install: $(GETTEXT_PACKAGE).1
	mkdir -p $(MANDIR)
	gzip -c $^ > $(MANDIR)/$(GETTEXT_PACKAGE).gz


clean:
	
