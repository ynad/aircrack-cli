## Aircrack-CLI Makefile

target:
	# Main module - Object
	gcc -Wall -I src/ -c src/main.c src/lib.c src/install.c
	# Secondary module - Object
	gcc -Wall -I src/ -c src/airjammer.c src/lib.c src/install.c -lpthread
	mkdir -p obj
	mv *.o obj

debug:
	# Main module - DEBUG mode
	gcc -Wall -I src/ -c src/main.c src/lib.c src/install.c -DDEBUG -g
	# Secondary module - DEBUG mode
	gcc -Wall -I src/ -c src/airjammer.c src/lib.c src/install.c -lpthread -DDEBUG -g
	mkdir -p obj
	mv *.o obj

install: target
	mkdir -p bin
	# Main module - Executable
	gcc obj/main.o obj/lib.o obj/install.o -o bin/aircrack-cli.bin
	# Secondary module - Executable
	gcc obj/airjammer.o obj/lib.o obj/install.o -o bin/airjammer.bin -lpthread

dbginstall: debug
	mkdir -p bin
	# Main module - Executable
	gcc obj/main.o obj/lib.o obj/install.o -o bin/aircrack-cli.bin
	# Secondary module - Executable
	gcc obj/airjammer.o obj/lib.o obj/install.o -o bin/airjammer.bin -lpthread

clean:
	rm -rf obj

uninstall: clean
	rm -f bin/*
	test -h bin || rmdir bin

