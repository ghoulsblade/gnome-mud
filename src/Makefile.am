GMUD_DEBUG = -Wall -O0 -g

localedir = $(datadir)/locale
INCLUDES = -DLOCALEDIR=\"$(localedir)\" \
	   -DPIXMAPSDIR=\""$(datadir)/pixmaps"\" \
	   -DPREFIX=\""$(prefix)"\" \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" \
	   -DDATADIR=\""$(datadir)"\" \
	   -DLIBDIR=\""$(libdir)"\" \
	   -DGTK_ENABLE_BROKEN=1 \
	   $(GMUD_CFLAGS) $(GMUD_DEBUG)
LDADD   = $(GMUD_LIBS) $(Z_LIBS)
AM_CPPFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"

gamesdir = $(prefix)/games
games_PROGRAMS = gnome-mud

gnome_mud_SOURCES = data.c init.c gnome-mud.c gnome-mud.h keybind.c log.c \
		    map.c map.h map-link.c mccpDecompress.c mccpDecompress.h \
		    modules.c modules.h modules_api.c modules_api.h mudlist.c \
		    net.c prefs.c profiles.c python.c telnet.c window.c
