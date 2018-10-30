all: run

main: P3_2.o
	gcc -Wall -o main problem3_2.c -lpthread

P3_2.o: problem3_2.c
	gcc -Wall -c problem3_2.c

run: main
	./main -n 10

clean:
	rm *.o
