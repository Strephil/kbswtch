PREFIX?=/usr/local

DEFAULT_LIBS=-lX11
LIBS=`pkg-config --libs x11 2>/dev/null || echo $(DEFAULT_LIBS)`

LDFLAGS+=$(LIBS)
CFLAGS?=-pipe --std=c99 -Os

all: kbswtch

install: kbswtch
	cp kbswtch $(PREFIX)/bin

uninstall:
	rm -f $(PREFIX)/bin/kbswtch

clean:
	rm -f kbswtch *.o

kbswtch: kbswtch.o tablemap.o options.o
	$(CC) $(CFLAGS) $(LDFLAGS) kbswtch.o tablemap.o  options.o -o $@

kbswtch.o: kbswtch.c
	$(CC) $(CFLAGS) -c kbswtch.c
tablemap.o: tablemap.c tablemap.h
	$(CC) $(CFLAGS) -c tablemap.c
options.o: options.c
	$(CC) $(CFLAGS) -c options.c 
