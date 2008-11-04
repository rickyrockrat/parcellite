desktopdir = $(datadir)/applications
desktop_in_file = parcellite.desktop.in
desktop_DATA = $(desktop_in_file:.desktop.in=.desktop)

autostartdir = /etc/xdg/autostart
autostart_in_file = parcellite-startup.desktop.in
autostart_DATA = $(autostart_in_file:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
  $(desktop_DATA) \
  $(autostart_DATA) \
  $(desktop_in_file) \
  $(autostart_in_file)

DISTCLEANFILES = \
  $(desktop_DATA) \
  $(autostart_DATA)


