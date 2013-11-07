all: orderSearcher

orderSearcher: orderSearcher.c
	gcc -Wall orderSearcher.c -o orderSearcher -lpthread

gdb: orderSearcher.c
	gcc -Wall -g orderSearcher.c -o orderSearcher -lpthread

clean:
	rm orderSearcher
