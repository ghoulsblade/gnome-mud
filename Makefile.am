SUBDIRS    = doc po src plugins

schemadir = $(sysconfdir)/gconf/schemas
schema_in_files = gnome-mud.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

Applicationsdir = $(datadir)/gnome/apps/Games
Applications_in_files = gnome-mud.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = ABOUT-NLS AUTHORS BUGS COPYING ChangeLog INSTALL NEWS PLUGIN.API \
		README ROADMAP config.rpath configure.in gnome-mud.desktop.in \
		gnome-mud.lsm intltool-extract.in intltool-merge.in \
		intltool-update.in $(Applications_DATA) $(schema_DATA)

install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)

