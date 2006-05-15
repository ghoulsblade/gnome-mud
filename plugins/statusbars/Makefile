CC=gcc
OBJS=main.o
PROG=statusbars.plugin
CFLAGS=-DBUILDING_PLUGIN -g -Wall `pkg-config --cflags glib-2.0 gtk+-2.0 libgnetwork-1.0 libpcre` 
LDFLAGS=-shared -fPIC

all:	$(OBJS)
	$(CC) $(OBJS) -o $(PROG) $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(PROG)

distdir:

