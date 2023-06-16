all: 	escalonador.o funcs.o
	gcc -o escalonador escalonador.o funcs.o
	rm -rf *.o

escalonador.o: escalonador.c
	gcc -c escalonador.c

funcs.o: funcs.c
	gcc -c funcs.c
	
