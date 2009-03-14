localedir = $(datadir)/locale
INCLUDES =								\
	-DLOCALEDIR=\"$(localedir)\"					\
	-DPIXMAPSDIR=\""$(datadir)/pixmaps"\"				\
	-DGMPIXMAPSDIR=\""$(datadir)/icons/hicolor/scalable/apps"\"	\
	-DPREFIX=\""$(prefix)"\"					\
	-DSYSCONFDIR=\""$(sysconfdir)"\"				\
	-DDATADIR=\""$(datadir)"\"					\
	-DLIBDIR=\""$(libdir)"\"					\
	-DGLADEDIR=\""$(pkgdatadir)"\"					\
	$(GMUD_CFLAGS) $(MSP_CFLAGS)
LDADD   = $(GMUD_LIBS) $(Z_LIBS) $(MSP_LIBS)
AM_CPPFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\"

bin_PROGRAMS = gnome-mud

gnome_mud_SOURCES =				\
	gconf-helper.c				\
	gconf-helper.h				\
	debug-logger.c				\
	debug-logger.h				\
	gnome-mud.c				\
	gnome-mud.h				\
	gnome-mud-icons.h			\
	mud-connection-view.c			\
	mud-connections.c			\
	mud-connections.h			\
	mud-connection-view.h			\
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
	mud-telnet-handler-interface.c		\
	mud-telnet-handler-interface.h		\
	mud-telnet-ttype.c			\
	mud-telnet-ttype.h			\
	mud-telnet-naws.c			\
	mud-telnet-naws.h			\
	mud-telnet-echo.c			\
	mud-telnet-echo.h			\
	mud-telnet-eor.c			\
	mud-telnet-eor.h			\
	mud-telnet-charset.c			\
	mud-telnet-charset.h			\
	mud-telnet-mccp.c			\
	mud-telnet-mccp.h			\
	mud-telnet-msp.c			\
	mud-telnet-msp.h			\
	mud-telnet-mssp.c			\
	mud-telnet-mssp.h			\
	mud-telnet-zmp.c			\
	mud-telnet-zmp.h			\
	mud-tray.c				\
	mud-tray.h				\
	mud-window.c				\
	mud-window.h				\
	mud-window-profile.c			\
	mud-window-profile.h			\
	utils.c					\
	utils.h

