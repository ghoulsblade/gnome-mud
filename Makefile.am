SUBDIRS    = doc po src plugins

Applicationsdir = $(datadir)/gnome/apps/Games
Applications_in_files = gnome-mud.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = ABOUT-NLS AUTHORS BUGS COPYING ChangeLog INSTALL NEWS PLUGIN.API \
		README ROADMAP config.rpath configure.in gnome-mud.desktop.in \
		gnome-mud.lsm intltool-extract.in, intltool-merge.in \
		intltool-update.in $(Applications_DATA)
