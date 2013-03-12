AM_CFLAGS = -I$(top_srcdir) -DPARCELLITELOCALEDIR=\""$(parcellitelocaledir)"\"
INCLUDES = $(GTK_CFLAGS) $(APPINDICATOR_CFLAGS)
LDADD = $(GTK_LIBS) $(APPINDICATOR_LIBS) -lX11 -lgdk-x11-2.0 -lpango-1.0 -lgobject-2.0 -lglib-2.0

bin_PROGRAMS = parcellite

DISTCLEANFILES = *.bak *.log *~ .deps/*.P

parcellite_SOURCES = main.c main.h \
                     utils.c utils.h \
                     parcellite-i18n.h\
                     daemon.c daemon.h \
                     history.c history.h \
                     keybinder.c keybinder.h \
                     preferences.c preferences.h \
                     eggaccelerators.c eggaccelerators.h \
                     attr_list.c attr_list.h
                     
$(top_srcdir)/config.h: $(parcellite_SOURCES)
	GO=$$(grep -c svnversion $(top_srcdir)/configure.ac); if [ "0" != "$$GO" ]; then \
	VER=$$($(top_srcdir)/svnversion.sh); sed -i "s#\(.*\)svn.*\".*#\1$$VER\"#" $(top_srcdir)/config.h; fi
	
