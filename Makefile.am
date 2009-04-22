SUBDIRS = data doc po src ui

@INTLTOOL_SCHEMAS_RULE@
@INTLTOOL_DESKTOP_RULE@

schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_in_files = gnome-mud.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

Applicationsdir = $(datadir)/applications
Applications_in_files = gnome-mud.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)

# Removed gnome-doc-utils.make. Will need to be replaced with
# updated docs.

EXTRA_DIST = \
	ABOUT-NLS \
	AUTHORS \
	BUGS \
	COPYING \
	ChangeLog \
	INSTALL \
	NEWS \
	README \
	ROADMAP \
	config.rpath \
	gnome-mud.desktop.in \
	gnome-mud.spec \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(Applications_DATA) \
	$(schema_DATA)

DISTCLEANFILES =		\
	intltool-extract	\
	intltool-merge		\
	intltool-update

MAINTAINERCLEANFILES = \
	INSTALL \
	aclocal.m4 \
	compile \
	config.guess \
	config.h.in \
	config.sub \
	configure.scan \
	depcomp \
	install-sh \
	ltmain.sh \
	missing \
	mkinstalldirs \
	gnome-mud.desktop \
	gnome-mud.schemas


DISTCHECK_CONFIGURE_FLAGS = --disable-schemas-install --disable-scrollkeeper

install-data-local: $(schema_DATA)
if GCONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)"; then \
		for p in $^ ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
		done \
	fi
endif

-include $(top_srcdir)/git.mk
