# Copyright (C) 2007-2022 Doug Springer <gpib@rickyrockrat.net>
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
PRFX=..
BUILDCONFIG=
export PRFX BUILDCONFIG
include ../simple.common

CFILES=attr_list.c daemon.c eggaccelerators.c  history.c keybinder.c main.c preferences.c utils.c

OBJS=$(patsubst %.c,%.o,$(CFILES))
GTKC=$(shell pkg-config --cflags gtk+-3.0)
FONTC=$(shell pkg-config --cflags fontconfig)
FREEC=$(shell pkg-config --cflags freetype2)
LIBAPP=$(shell $(PRFX)/appsuck.sh cflags)


CFLAGS=$(GTKC) $(FONTC) $(FREEC) -DHAVE_CONFIG_H  -DGETTEXT_PACKAGE='"parcellite"' -DENABLE_NLS=1 -I. -I.. -DPARCELLITELOCALEDIR=\""/usr/share/locale"\" -Wno-deprecated -Wno-deprecated-declarations

$(GETTEXT_PACKAGE): check_dep $(OBJS)
	gcc -Wall $(OBJS) -o $@ $(LIBS)

%.:%.c
	gcc -Wall $CFLAGS $< -o $@

install: $(GETTEXT_PACKAGE)
	mkdir -p $(INSTDIR)/bin	
	cp $(GETTEXT_PACKAGE) $(INSTDIR)/bin

clean:
	rm $(OBJS) $(GETTEXT_PACKAGE)
