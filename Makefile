## Aircrack-CLI Makefile

target:
	# Main module - Object
	gcc -Wall -I src/ -c src/main.c src/lib.c src/install.c
	# Secondary module - Object
	gcc -Wall -I src/ -c src/airjammer.c src/lib.c src/install.c -lpthread
	mv *.o obj

debug:
	# Main module - DEBUG mode
	gcc -Wall -I src/ -c src/main.c src/lib.c src/install.c -DDEBUG -g
	# Secondary module - DEBUG mode
	gcc -Wall -I src/ -c src/airjammer.c src/lib.c src/install.c -lpthread -DDEBUG -g
	mv *.o obj

install: target
	# Main module - Executable
	gcc obj/main.o obj/lib.o obj/install.o -o bin/aircrack-cli.bin
	# Secondary module - Executable
	gcc obj/airjammer.o obj/lib.o obj/install.o -o bin/airjammer.bin -lpthread

dbginstall: debug
	# Main module - Executable
	gcc obj/main.o obj/lib.o obj/install.o -o bin/aircrack-cli.bin
	# Secondary module - Executable
	gcc obj/airjammer.o obj/lib.o obj/install.o -o bin/airjammer.bin -lpthread

clean:
	rm obj/*
	rm bin/airjammer.bin bin/aircrack-cli.bin
