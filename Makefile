# Copyright (C) 2010-2024 Doug Springer <gpib@rickyrockrat.net>
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

prefix?=/usr
DESTDIR?=
sysconfdir = $(DESTDIR)/etc
INSTDIR:=$(DESTDIR)$(prefix)
# Attempt to help intltool on some systems (not needed on Alpine).
top_builddir:=$(CURDIR)
export top_builddir


GETTEXT_PACKAGE:=parcellite

DIRS:= data po src doc

all: src/$(GETTEXT_PACKAGE)
BUILDCONFIG=yes
PRFX=.
export PRFX
include simple.common

GIT_PVERSION:=$(shell git log -n1 --pretty=format:%h)
VERSION?=git-$(GIT_PVERSION)




makeopts=-f Makefile GETTEXT_PACKAGE=$(GETTEXT_PACKAGE) BUILDCONFIG=''

config.h:
	@echo "#define ENABLE_NLS 1" > $@
	@echo '#define GETTEXT_PACKAGE "parcellite"' >> $@
	@echo '#define VERSION "$(VERSION)"' >> $@
	

build.po build.data: config.h
	$(MAKE) -C $$(echo $@|sed 's!build.!!') $(makeopts)

src/$(GETTEXT_PACKAGE): check_dep build.po build.data 
	$(MAKE) -C src $(makeopts) $(GETTEXT_PACKAGE)

install-bin: src/$(GETTEXT_PACKAGE) build.data
	$(MAKE) -C data $(makeopts) sysconfdir=$(sysconfdir) INSTDIR=$(INSTDIR) GETTEXT_PACKAGE=$(GETTEXT_PACKAGE) install
	$(MAKE) -C src $(makeopts) sysconfdir=$(sysconfdir) INSTDIR=$(INSTDIR) GETTEXT_PACKAGE=$(GETTEXT_PACKAGE) install

install-lang: build.po
	$(MAKE) -C po $(makeopts) sysconfdir=$(sysconfdir) INSTDIR=$(INSTDIR) GETTEXT_PACKAGE=$(GETTEXT_PACKAGE) install

install-doc:	
	$(MAKE) -C doc $(makeopts) sysconfdir=$(sysconfdir) INSTDIR=$(INSTDIR) GETTEXT_PACKAGE=$(GETTEXT_PACKAGE) install
	mkdir -p $(INSTDIR)/share/doc/$(GETTEXT_PACKAGE)
	install -c -m 644 COPYING ChangeLog README $(INSTDIR)/share/doc/$(GETTEXT_PACKAGE)
	
install: src/$(GETTEXT_PACKAGE) build.po build.data
	for d in $(DIRS); do \
	$(MAKE) -C $$d $(makeopts) sysconfdir=$(sysconfdir) INSTDIR=$(INSTDIR) GETTEXT_PACKAGE=$(GETTEXT_PACKAGE) $@; \
	done

clean: 
	-for d in $(DIRS); do \
	$(MAKE) -C $$d $(makeopts)  $@ ; \
	done
	-rm config.h build.po build.data src/config.simple.h

