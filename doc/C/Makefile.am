
SGML_FILES = gnome-mud.sgml

EXTRA_DIST = $(SGML_FILES) topic.dat

figs =	preferences-window.png	\
	main-window.png		\
	profiles-window.png

docname = gnome-mud
lang = C
omffile = gnome-mud-C.omf
sgml_ents = $(SGML_FILES)

include $(top_srcdir)/doc/sgmldocs.make
dist-hook: app-dist-hook
