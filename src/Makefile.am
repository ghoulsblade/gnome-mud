EXTRA_DIST        = gnome-mud.c

GMUD_DEBUG = -Wall -O0 -g

localedir = $(datadir)/locale
INCLUDES = -DLOCALEDIR=\"$(localedir)\" \
	   -DPIXMAPSDIR=\"$(datadir)/pixmaps\" \
	   -DPREFIX="\"$(prefix)"\" \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" \
	   -DDATADIR=\""$(datadir)"\" \
	   -DLIBDIR=\""$(libdir)"\" \
	   -DGTK_ENABLE_BROKEN=1 $(GMUD_CFLAGS) $(GMUD_DEBUG)

gamesdir = $(prefix)/games
games_PROGRAMS = gnome-mud

gnome_mud_SOURCES = data.c init.c keybind.c map.c map-link.c \
                    net.c prefs.c window.c modules.c modules_api.c \
                    modules.h modules_api.h gnome-mud.h telnet.c \
		    mccpDecompress.c mccpDecompress.h profiles.c python.c \
		    mudlist.c log.c map.h
gnome_mud_LDADD   = gnome-mud.o $(GMUD_LIBS) $(Z_LIBS)

gnome-mud.o: gnome-mud.c
	$(COMPILE) -DPKGDATADIR=\"$(pkgdatadir)\" -c $(top_srcdir)/src/gnome-mud.c
