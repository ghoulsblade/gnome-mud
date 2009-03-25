NULL =

noinst_LTLIBRARIES = libtelnethandlers.la

libtelnethandlers_la_SOURCES = \
	mud-telnet-charset.c \
	mud-telnet-charset.h \
	mud-telnet-echo.c \
	mud-telnet-echo.h \
	mud-telnet-eor.c \
	mud-telnet-eor.h \
	mud-telnet-handlers.h \
	mud-telnet-handler-interface.c \
	mud-telnet-handler-interface.h \
	mud-telnet-mccp.c \
	mud-telnet-mccp.h \
	mud-telnet-msp.c \
	mud-telnet-msp.h \
	mud-telnet-mssp.c \
	mud-telnet-mssp.h \
	mud-telnet-naws.c \
	mud-telnet-naws.h \
	mud-telnet-new-environ.c \
	mud-telnet-new-environ.h \
	mud-telnet-ttype.c \
	mud-telnet-ttype.h \
	mud-telnet-zmp.c \
	mud-telnet-zmp.h \
	$(NULL)

libtelnethandlers_la_CPPFLAGS = \
	-I$(srcdir)/.. \
	$(AM_CPPFLAGS)

libtelnethandlers_la_CFLAGS = \
	$(GMUD_CFLAGS) \
	$(MSP_CFLAGS) \
	-g -Wall -O0

libtelnethandlers_la_LDFLAGS =

libtelnethandlers_la_LIBADD = \
	$(GMUD_LIBS) \
	$(MSP_LIBS) \
	$(Z_LIBS)

