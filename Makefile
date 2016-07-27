
all: csim.o csim

csim.o:
	gcc -c csim.c

csim:
	gcc csim.o -o csim

clean:
	rm -rf *.o
	rm -f csim
