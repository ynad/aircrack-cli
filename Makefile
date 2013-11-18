target:
	# object for main executable
	gcc -Wall -I src/ -c src/main.c src/lib.c src/install.c
	# object for secondary module
	gcc -Wall -I src/ -c src/airjammer.c src/lib.c
	mv *.o obj

install: target
	# main executable
	gcc obj/main.o obj/lib.o obj/install.o -o bin/aircrack-cli.bin
	# secondary module executable
	gcc obj/airjammer.o obj/lib.o -o bin/airjammer.bin

clean:
	rm obj/*
	rm bin/*
