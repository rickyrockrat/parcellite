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

DATADIR:=$(INSTDIR)/share

desktopdir = $(DATADIR)/applications
pixmapdir = $(DATADIR)/pixmaps
autostartdir = $(sysconfdir)/xdg/autostart
top_srcdir = ..

srcdir:=.

# po dir
WINST = $(shell which install)
INSTALL = $(WINST) -c
INSTALL_DATA = ${INSTALL} -m 644
GMSGFMT = $(shell which msgfmt)
MSGFMT = $(GMSGFMT)
XGETTEXT = $(shell which xgettext)
INTLTOOL_UPDATE = $(shell which intltool-update)
INTLTOOL_EXTRACT = $(shell which intltool-extract)
MSGMERGE = INTLTOOL_EXTRACT="$(INTLTOOL_EXTRACT)" XGETTEXT="$(XGETTEXT)" srcdir=$(srcdir) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE) --dist
GENPOT   = INTLTOOL_EXTRACT="$(INTLTOOL_EXTRACT)" XGETTEXT="$(XGETTEXT)" srcdir=$(srcdir) $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE) --pot
INTLTOOL_MERGE = $(shell which intltool-merge)


# installable items
ICONS:=	$(GETTEXT_PACKAGE).png $(GETTEXT_PACKAGE).svg $(GETTEXT_PACKAGE).xpm
DESK:= $(GETTEXT_PACKAGE).desktop $(GETTEXT_PACKAGE)-startup.desktop

all: $(GETTEXT_PACKAGE).desktop $(GETTEXT_PACKAGE).png


%.desktop: %.desktop.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po)
	LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_OPTIONS) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

	
install: $(ICONS) $(DESK)
	mkdir -p $(autostartdir) $(desktopdir)
	@echo "Installing $(GETTEXT_PACKAGE).desktop to $(desktopdir)"
	$(INSTALL_DATA) $(GETTEXT_PACKAGE).desktop $(desktopdir)
	@echo "Installing $(GETTEXT_PACKAGE)-startup.desktop to $(autostartdir)"
	$(INSTALL_DATA) $(GETTEXT_PACKAGE)-startup.desktop $(autostartdir)
	mkdir -p $(pixmapdir)
	for i in $(ICONS); do echo "Installing $$i to $(pixmapdir)"; \
		$(INSTALL_DATA) $$i $(pixmapdir); \
	done

clean:
	-rm *.desktop
