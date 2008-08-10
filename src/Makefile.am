AM_CFLAGS = -I$(top_srcdir) -DPARCELLITELOCALEDIR=\""$(parcellitelocaledir)"\"
INCLUDES = $(GTK_CFLAGS)
LDADD = $(GTK_LIBS)

bin_PROGRAMS = parcellite

DISTCLEANFILES = *.bak *.log *~ .deps/*.P

parcellite_SOURCES = main.c main.h \
                     utils.c utils.h \
                     history.c history.h \
                     keybinder.c keybinder.h \
                     preferences.c preferences.h \
                     eggaccelerators.c eggaccelerators.h
