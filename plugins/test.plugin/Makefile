CC=gcc
OBJS=main.o
PROG=test.plugin
CFLAGS=-DBUILDING_PLUGIN -g -Wall -I../../ -I/usr/lib/glib/include/ `gtk-config --cflags`
LDFLAGS=-shared -fPIC -L/usr/local/lib -lglib12  -L/usr/X11R6/lib -lgtk12

all:	$(OBJS)
	$(CC) $(OBJS) -o $(PROG) $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(PROG)

distdir:

