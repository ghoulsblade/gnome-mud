SUBDIRS = pixmaps

GMUD_DEBUG = -Wall -O0 -g

localedir = $(datadir)/locale
INCLUDES =							\
	-DLOCALEDIR=\"$(localedir)\"				\
	-DPIXMAPSDIR=\""$(datadir)/pixmaps"\"			\
	-DGMPIXMAPSDIR=\""$(datadir)/pixmaps/gnome-mud"\"	\
	-DPREFIX=\""$(prefix)"\"				\
	-DSYSCONFDIR=\""$(sysconfdir)"\"			\
	-DDATADIR=\""$(datadir)"\"				\
	-DLIBDIR=\""$(libdir)"\"				\
	-DGLADEDIR=\"../ui/\" \
	$(GMUD_CFLAGS) $(GMUD_DEBUG)
LDADD   = $(GMUD_LIBS) $(Z_LIBS)
AM_CPPFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"

gamesdir = $(prefix)/games
games_PROGRAMS = gnome-mud

gnome_mud_SOURCES =		\
	gconf-helper.c \
	gconf-helper.h \
	gnome-mud.c \
	gnome.mud.h \
	mud-connection-view.c \
	mud-connection-view.h \
	mud-preferences-window.c \
	mud-preferences-window.h \
	mud-profile.c \
	mud-profile.h \
	mud-window.c \
	mud-window.h
	
#	data.c			\
#	eggtrayicon.c		\
#	eggtrayicon.h		\
#	init.c			\
#	gnome-mud.c		\
#	gnome-mud.h		\
#	keybind.c		\
#	log.c			\
#	map.c			\
#	map.h			\
#	map-link.c		\
#	mccpDecompress.c	\
#	mccpDecompress.h	\
#	modules.c		\
#	modules.h		\
#	modules_api.c		\
#	modules_api.h		\
#	mudlist.c		\
#	net.c			\
#	prefs.c			\
#	profiles.c		\
#	python.c		\
#	telnet.c		\
#	tray.c			\
#	window.c
