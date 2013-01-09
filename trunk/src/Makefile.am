AM_CFLAGS = -I$(top_srcdir) -DPARCELLITELOCALEDIR=\""$(parcellitelocaledir)"\"
INCLUDES = $(GTK_CFLAGS)
LDADD = $(GTK_LIBS) -lX11 -lgdk-x11-2.0 -lpango-1.0 -lgobject-2.0 -lglib-2.0

bin_PROGRAMS = parcellite

DISTCLEANFILES = *.bak *.log *~ .deps/*.P

parcellite_SOURCES = main.c main.h \
                     utils.c utils.h \
                     parcellite-i18n.h\
                     daemon.c daemon.h \
                     history.c history.h \
                     keybinder.c keybinder.h \
                     preferences.c preferences.h \
                     eggaccelerators.c eggaccelerators.h
