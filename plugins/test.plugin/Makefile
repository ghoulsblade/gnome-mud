CC=gcc
OBJS=main.o
PROG=test.plugin
CFLAGS=-DBUILDING_PLUGIN -g -Wall -fPIC `pkg-config --cflags glib-2.0 gtk+-2.0 gnet-2.0 gconf-2.0 libpcre` 
LDFLAGS=-fPIC -shared

all:	$(OBJS)
	$(CC) $(OBJS) -o $(PROG) $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(PROG)

distdir:

