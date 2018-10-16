all: main

main: coins.o
	gcc -Wall -o main flip_coins.c -lpthread

coins.o: flip_coins.c
	gcc -Wall -c flip_coins.c

clean:
	rm *.o