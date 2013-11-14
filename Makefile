target:
	mkdir obj
	mkdir bin
	gcc -c main.c install.c -I . -Wall
	mv *.o obj
	gcc obj/*.o -o bin/aircrack-cli.bin

clean:
	rm obj/*
	rm bin/*
	rmdir obj
	rmdir bin
