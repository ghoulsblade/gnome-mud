localedir = $(datadir)/locale
CFLAGS	         += $(DEBUG_FLAGS)
INCLUDES          = -DLOCALEDIR=\"$(localedir)\" $(GNOME_INCLUDEDIR)
EXTRA_DIST        = gnome-mud.c
bin_PROGRAMS   	  = gnome-mud
gnome_mud_SOURCES  = data.c color.c init.c keybind.c map.c \
		    		net.c prefs.c window.c wizard.c dialog.c version.c \
                    modules.c modules_api.c modules.h modules_api.h gnome-mud.h \
		    		readme_doc.h authors_doc.h telnet.c mccpDecompress.c \
		    		mccpDecompress.h profiles.c
gnome_mud_LDADD    = gnome-mud.o $(GNOME_LIBDIR) $(GNOMEUI_LIBS)

gnome-mud.o: gnome-mud.c
	$(COMPILE) -DPKGDATADIR=\"$(pkgdatadir)\" -c $(top_srcdir)/src/gnome-mud.c
