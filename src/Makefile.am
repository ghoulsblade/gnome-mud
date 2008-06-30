SUBDIRS = pixmaps svg

GMUD_DEBUG = -Wall -O0 -g

localedir = $(datadir)/locale
INCLUDES =												\
	-DLOCALEDIR=\"$(localedir)\"						\
	-DPIXMAPSDIR=\""$(datadir)/pixmaps"\"				\
	-DGMPIXMAPSDIR=\""$(datadir)/pixmaps/gnome-mud"\"	\
	-DPREFIX=\""$(prefix)"\"							\
	-DSYSCONFDIR=\""$(sysconfdir)"\"					\
	-DDATADIR=\""$(datadir)"\"							\
	-DLIBDIR=\""$(libdir)"\"							\
	-DGLADEDIR=\""$(pkgdatadir)"\"						\
	$(GMUD_CFLAGS) $(GMUD_DEBUG)
LDADD   = $(GMUD_LIBS) $(Z_LIBS)
AM_CPPFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"

gamesdir = $(prefix)/games
games_PROGRAMS = gnome-mud

gnome_mud_SOURCES =				\
	gconf-helper.c				\
	gconf-helper.h				\
	gnome-mud.c					\
	gnome.mud.h					\
	modules_api.c				\
	modules_api.h				\
	modules.c					\
	modules.h					\
	modules-structures.h		\
	mud-connection-view.c		\
	mud-connection-view.h		\
	mud-log.c					\
	mud-log.h					\
	mud-parse-alias.c			\
	mud-parse-alias.h			\
	mud-parse-base.c			\
	mud-parse-base.h			\
	mud-parse-trigger.c			\
	mud-parse-trigger.h			\
	mud-preferences-window.c	\
	mud-preferences-window.h	\
	mud-profile.c				\
	mud-profile.h				\
	mud-regex.c					\
	mud-regex.h					\
	mud-telnet.c                \
	mud-telnet.h                \
	mud-telnet-handlers.c		\
	mud-telnet-handlers.h		\
	mud-telnet-msp.c			\
	mud-telnet-msp.h			\
	mud-telnet-zmp.c			\
	mud-telnet-zmp.h			\
	mud-tray.c					\
	mud-tray.h					\
	mud-window.c				\
	mud-window.h				\
	mud-window-mconnect.h		\
	mud-window-mconnect.c		\
	mud-window-mudedit.c		\
	mud-window-mudedit.h		\
	mud-window-mudlist.c		\
	mud-window-mudlist.h		\
	mud-window-profile.c		\
	mud-window-profile.h		\
	utils.c						\
	utils.h
