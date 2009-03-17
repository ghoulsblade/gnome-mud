NULL =

noinst_LTLIBRARIES = libzmpcommands.la

libzmpcommands_la_SOURCES = \
	zmp-subwindow.c \
	zmp-subwindow.h \
	$(NULL)

libzmpcommands_la_CPPFLAGS = \
	-I$(srcdir)/.. \
	$(AM_CPPFLAGS)

libzmpcommands_la_CFLAGS = \
	$(GMUD_CFLAGS) \
	$(MSP_CFLAGS)

libzmpcommands_la_LDFLAGS =

libzmpcommands_la_LIBADD = \
	$(GMUD_LIBS) \
	$(MSP_LIBS) \
	$(Z_LIBS)

