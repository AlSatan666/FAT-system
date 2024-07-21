CC = gcc
CFLAGS = -w -std=c99

all: myfs

myfs: main.o file_system.o
	$(CC) $(CFLAGS) -o myfs main.o file_system.o

main.o: main.c file_system.h
	$(CC) $(CFLAGS) -c main.c

file_system.o: file_system.c file_system.h
	$(CC) $(CFLAGS) -c file_system.c

clean:
	rm -f myfs *.o

run: myfs
	./myfs
