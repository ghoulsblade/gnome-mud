EXTRA_DIST     = amcl.c mkhelp.c

bin_PROGRAMS   = amcl

amcl_SOURCES   = action.c alias.c color.c init.c keybind.c map.c misc.c \
		 net.c prefs.c window.c wizard.c dialog.c version.c \
                 modules.c modules_api.c modules.h modules_api.h amcl.h 

amcl_LDADD	= amcl.o

amcl.o: amcl.c
	$(COMPILE) -DPKGDATADIR=\"$(pkgdatadir)\" -c amcl.c

readme_doc.h: ../README mkhelp
	@(cd ..; src/mkhelp README src/readme_doc.h)

authors_doc.h: ../AUTHORS mkhelp
	@(cd ..; src/mkhelp AUTHORS src/authors_doc.h)

version.h: FORCE
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

FORCE:	


mkhelp: mkhelp.c
	$(COMPILE) -o mkhelp mkhelp.c
