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
EXECUTABLE1=bin/aircrack-cli.bin
EXECUTABLE2=bin/airjammer.bin

target:
	# Main module - Object
	$(CC) $(CFLAGS) $(INCL) -c $(SOURCES1)
	# Secondary module - Object
	$(CC) $(CFLAGS) $(INCL) -c $(SOURCES2) $(LDFLAGS)
	mkdir -p obj
	mv *.o obj

install: target
	mkdir -p bin
	# Main module - Executable
	$(CC) $(CFLAGS) $(OBJECTS1) -o $(EXECUTABLE1)
	# Secondary module - Executable
	$(CC) $(CFLAGS) $(OBJECTS2) -o $(EXECUTABLE2) $(LDFLAGS)

debug:
	mkdir -p obj
	mkdir -p bin
	# Main module - DEBUG mode
	$(CC) $(CFLAGS) $(INCL) -c $(SOURCES1) $(DEBUG)
	# Secondary module - DEBUG mode
	$(CC) $(CFLAGS) $(INCL) -c $(SOURCES2) $(LDFLAGS) $(DEBUG)
	mv *.o obj
	$(CC) $(CFLAGS) $(OBJECTS1) -o $(EXECUTABLE1) $(DEBUG)
	$(CC) $(CFLAGS) $(OBJECTS2) -o $(EXECUTABLE2) $(LDFLAGS) $(DEBUG)

.PHONY: clean

clean:
	rm -rf obj *.o

uninstall: clean
	rm -f bin/*
	test -h bin || rmdir bin

