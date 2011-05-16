#
#   cmusfm - makefile
#   Copyright (c) 2010 Arkadiusz Bokowy
#

CC      = gcc

CFLAGS  = -pipe -Wall -Os
LDFLAGS = -lcurl

OBJS    = main.o libscrobbler2.o server.o
PROG    = cmusfm

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(PROG)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $<

all: $(PROG)

clean:
	rm -f *.o
