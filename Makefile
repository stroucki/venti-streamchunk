ifndef VENTILIB
  VENTILIB=/usr/local/plan9/lib
endif

ifndef VENTIINC
  VENTIINC=/usr/local/plan9/include
endif

ifndef CFLAGS
  CFLAGS=-g -Wall
endif

main: streamchunkread.o streamchunkwrite.o msb.o rabinpoly.o
	$(CC) $(CFLAGS) -o streamchunkwrite -L $(VENTILIB) streamchunkwrite.o msb.o rabinpoly.o -lventi -lsec -lthread -l9 -lpthread
	$(CC) $(CFLAGS) -o streamchunkread -L $(VENTILIB) streamchunkread.o msb.o rabinpoly.o -lventi -lsec -lthread -l9 -lpthread

.c.o:
	$(CC) $(CFLAGS) -I $(VENTIINC) -c $<

clean:
	rm -f *.o streamchunkwrite streamchunkread
