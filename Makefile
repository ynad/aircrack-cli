## Aircrack-CLI Makefile

# Compilation variables
CC=gcc
CFLAGS=-Wall
LDFLAGS=-lpthread
INCL=-I src/
DEBUG=-g -DDEBUG
SOURCES1=src/aircrack-cli.c src/lib.c src/install.c
OBJECTS1=obj/aircrack-cli.o obj/lib.o obj/install.o
SOURCES2=src/airjammer.c src/lib.c src/install.c
OBJECTS2=obj/airjammer.o obj/lib.o obj/install.o
BINS=bin/
EXECUTABLE1=aircrack-cli.bin
EXECUTABLE2=airjammer.bin
SYSPATH=/usr/local/sbin/

target:
	# Make objects
	$(CC) $(CFLAGS) $(INCL) -c $(SOURCES1)
	$(CC) $(CFLAGS) $(INCL) -c $(SOURCES2) $(LDFLAGS)
	mkdir -p obj
	mv *.o obj
	mkdir -p bin
	# Make executables
	$(CC) $(CFLAGS) $(OBJECTS1) -o $(BINS)$(EXECUTABLE1)
	$(CC) $(CFLAGS) $(OBJECTS2) -o $(BINS)$(EXECUTABLE2) $(LDFLAGS)

install:
	cp $(BINS)$(EXECUTABLE1) $(BINS)$(EXECUTABLE2) $(SYSPATH)

debug:
	mkdir -p obj
	mkdir -p bin
	# Main module - DEBUG mode
	$(CC) $(CFLAGS) $(INCL) -c $(SOURCES1) $(DEBUG)
	# Secondary module - DEBUG mode
	$(CC) $(CFLAGS) $(INCL) -c $(SOURCES2) $(LDFLAGS) $(DEBUG)
	mv *.o obj
	$(CC) $(CFLAGS) $(OBJECTS1) -o $(BINS)$(EXECUTABLE1) $(DEBUG)
	$(CC) $(CFLAGS) $(OBJECTS2) -o $(BINS)$(EXECUTABLE2) $(LDFLAGS) $(DEBUG)

.PHONY: clean

clean:
	rm -rf obj *.o bin/*
	test -h bin || rm -rf bin

uninstall: clean
	rm $(SYSPATH)$(EXECUTABLE1) $(SYSPATH)$(EXECUTABLE2)

