SUBDIRS = doc po src plugins ui

schemadir = $(sysconfdir)/gconf/schemas
schema_in_files = gnome-mud.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

Applicationsdir = $(datadir)/applications
Applications_in_files = gnome-mud.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	ABOUT-NLS \
	AUTHORS \
	BUGS \
	COPYING \
	ChangeLog \
	INSTALL \
	NEWS \
	PLUGIN.API \
	README \
	ROADMAP \
	config.rpath \
	gnome-doc-utils.make \
	gnome-mud.desktop.in \
	gnome-mud.lsm \
	gnome-mud.spec \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(Applications_DATA) \
	$(schema_DATA)

DISTCLEANFILES =		\
	gnome-doc-utils.make	\
	intltool-extract	\
	intltool-merge		\
	intltool-update

DISTCHECK_CONFIGURE_FLAGS = --disable-schemas-install --disable-scrollkeeper

install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)

