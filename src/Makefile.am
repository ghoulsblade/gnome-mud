CFLAGS	         += $(DEBUG_FLAGS)
INCLUDES          = $(GNOME_INCLUDEDIR)
EXTRA_DIST        = amcl.c
bin_PROGRAMS   	  = gnomemud
gnomemud_SOURCES  = data.c color.c init.c keybind.c map.c \
		    net.c prefs.c window.c wizard.c dialog.c version.c \
                    modules.c modules_api.c modules.h modules_api.h amcl.h \
		    readme_doc.h authors_doc.h telnet.c mccpDecompress.c \
		    mccpDecompress.h
gnomemud_LDADD    = amcl.o $(GNOME_LIBDIR) $(GNOMEUI_LIBS)

amcl.o: amcl.c
	$(COMPILE) -DPKGDATADIR=\"$(pkgdatadir)\" -c $(top_srcdir)/src/amcl.c

version.h:
	@echo "/* this header is automatically generated */" > version.h
	@echo "/* and recreated for each new compilation */" >> version.h
	@echo "#define PROG_NAME \"$(PROGNAME)\"" >> version.h
	@echo "#define PROG_VERSION \"$(VERSION)\"" >> version.h
	@echo "#define COMPILE_DATE \"`date +%D`\"" >> version.h
	@echo "#define COMPILE_TIME \"`date +%T`\"" >> version.h
	@echo "#define COMPILE_BY \"`whoami`\"" >> version.h
	@echo "#define COMPILE_HOST \"`hostname`\"" >> version.h
	@echo "#define GTK_VERSION \"GTK+ `gtk-config --version`\"" >> version.h
	@echo "#define CC_VERSION \"`$(CC) -v 2>&1 | tail -1`\"">>version.h
	@echo "#define CODE_LINES `cat *.c amcl.h modules.h modules_api.h | grep -ch -e \"[[:alpha:]|[:punct:]]\"`">>version.h


