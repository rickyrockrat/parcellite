AUTOMAKE_OPTIONS = gnu

desktopdir = $(datadir)/applications
desktop_DATA = parcellite.desktop

autostartdir = /etc/xdg/autostart
autostart_DATA = parcellite-startup.desktop

SUBDIRS = doc src po

DISTCLEANFILES = *.bak *.log *~

EXTRA_DIST = \
	m4 \
	autogen.sh \
	config.rpath \
	m4/ChangeLog \
	$(desktop_DATA) \
	$(autostart_DATA) \
	intltool-merge.in \
	intltool-update.in \
	intltool-extract.in

ACLOCAL_AMFLAGS = -I m4
