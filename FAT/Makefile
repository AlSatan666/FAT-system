all: myfs

myfs:
	gcc -o myfs main.c file_system.c

clean:
	rm -f myfs *.o

run: myfs
	./myfs
