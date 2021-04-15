# Makefile for 3dpong

# by Bill Kendrick
# bill@newbreedsoftware.com

# New Breed Software
# http://www.newbreedsoftware.com/3dpng/

# December 9, 1997 - April 24, 2004


# Makefile user-definable variables
SDL_INCLUDE= -I/usr/include/SDL2
SDL_LIB=-lSDL2 -lSDL2_ttf

CC=gcc
CFLAGS=
MATHLIB=-lm


# Where to install things:

PREFIX=/usr/local
BIN_PREFIX=$(PREFIX)/bin
MAN_PREFIX=$(PREFIX)/man


# Makefile other variables

OBJECTS=obj/3dpong.o obj/randnum.o obj/text.o


# Makefile commands: 

all:	3dpong

install:	3dpong
	install -d $(BIN_PREFIX)
	cp 3dpong $(BIN_PREFIX)/
	chmod 755 $(BIN_PREFIX)/3dpong
	cp examples/3dpong-vs-computer.sh $(BIN_PREFIX)/
	chmod 755 $(BIN_PREFIX)/3dpong-vs-computer.sh
	cp examples/3dpong-handball.sh $(BIN_PREFIX)/
	chmod 755 $(BIN_PREFIX)/3dpong-handball.sh
	install -d $(MAN_PREFIX)/man6
	cp src/3dpong.6 $(MAN_PREFIX)/man6/
	chmod 644 $(MAN_PREFIX)/man6/3dpong.6

uninstall:
	-rm $(BIN_PREFIX)/3dpong
	-rm $(BIN_PREFIX)/3dpong-vs-computer.sh
	-rm $(BIN_PREFIX)/3dpong-handball.sh
	-rm $(MAN_PREFIX)/man6/3dpong.6

clean:
	-rm 3dpong
	-rm obj/*.o


# Application:

3dpong:	$(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(MATHLIB) $(SDL_LIB) -o 3dpong


# Application object:

obj/3dpong.o:	src/3dpong.c src/randnum.h src/text.h
	$(CC)	$(CFLAGS) $(SDL_INCLUDE) src/3dpong.c -c -o obj/3dpong.o


# Library objects:

obj/randnum.o:	src/randnum.c src/randnum.h
	$(CC)	$(CFLAGS) src/randnum.c -c -o obj/randnum.o

obj/text.o:	src/text.c src/text.h
	$(CC)	$(CFLAGS) $(SDL_INCLUDE) src/text.c -c -o obj/text.o

