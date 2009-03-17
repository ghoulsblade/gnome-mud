SUBDIRS = handlers zmp

bin_PROGRAMS = gnome-mud

localedir = $(datadir)/locale

gnome_mud_CPPFLAGS =							\
	-DLOCALEDIR=\"$(localedir)\"					\
	-DPIXMAPSDIR=\""$(datadir)/pixmaps"\"				\
	-DGMPIXMAPSDIR=\""$(datadir)/icons/hicolor/scalable/apps"\"	\
	-DPREFIX=\""$(prefix)"\"					\
	-DSYSCONFDIR=\""$(sysconfdir)"\"				\
	-DDATADIR=\""$(datadir)"\"					\
	-DLIBDIR=\""$(libdir)"\"					\
	-DGLADEDIR=\""$(pkgdatadir)"\"					\
	-DPKGDATADIR=\""$(pkgdatadir)"\"

gnome_mud_CFLAGS = \
	$(GMUD_CFLAGS) \
	$(MSP_CFLAGS)

gnome_mud_LDADD = \
	handlers/libtelnethandlers.la \
	zmp/libzmpcommands.la \
	$(GMUD_LIBS)	\
	$(Z_LIBS)	\
	$(MSP_LIBS)

gnome_mud_SOURCES =				\
	gconf-helper.c				\
	gconf-helper.h				\
	debug-logger.c				\
	debug-logger.h				\
	gnome-mud.c				\
	gnome-mud.h				\
	gnome-mud-marshallers.c			\
	gnome-mud-marshallers.h			\
	gnome-mud-icons.h			\
	mud-connection-view.c			\
	mud-connections.c			\
	mud-connections.h			\
	mud-connection-view.h			\
	mud-line-buffer.c			\
	mud-line-buffer.h			\
	mud-log.c				\
	mud-log.h				\
	mud-parse-alias.c			\
	mud-parse-alias.h			\
	mud-parse-base.c			\
	mud-parse-base.h			\
	mud-parse-trigger.c			\
	mud-parse-trigger.h			\
	mud-preferences-window.c		\
	mud-preferences-window.h		\
	mud-profile.c				\
	mud-profile.h				\
	mud-regex.c				\
	mud-regex.h				\
	mud-telnet.c				\
	mud-telnet.h				\
	mud-tray.c				\
	mud-tray.h				\
	mud-window.c				\
	mud-window.h				\
	mud-window-profile.c			\
	mud-window-profile.h			\
	utils.c					\
	utils.h

BUILT_SOURCES = \
	gnome-mud-marshallers.c \
	gnome-mud-marshallers.h

EXTRA_DIST = \
	gnome-mud-marshallers.list

CLEANFILES = \
	$(BUILT_SOURCES)

gnome-mud-marshallers.h: gnome-mud-marshallers.list
	( $(GLIB_GENMARSHAL) --prefix gnome_mud_cclosure --header \
	    gnome-mud-marshallers.list > gnome-mud-marshallers.h.tmp \
	    && mv gnome-mud-marshallers.h.tmp gnome-mud-marshallers.h ) \
	    || ( rm -f gnome-mud-marshallers.h.tmp && exit 1 )

gnome-mud-marshallers.c: gnome-mud-marshallers.list
	( $(GLIB_GENMARSHAL) --prefix gnome_mud_cclosure --body \
	    gnome-mud-marshallers.list > gnome-mud-marshallers.c.tmp \
	    && mv gnome-mud-marshallers.c.tmp gnome-mud-marshallers.c ) \
	    || ( rm -f gnome-mud-marshallers.c.tmp && exit 1 )

