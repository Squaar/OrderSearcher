all: orderSearcher

orderSearcher: orderSearcher.c
	gcc -Wall orderSearcher.c -o orderSearcher -lpthread -lm

gdb: orderSearcher.c
	gcc -Wall -g orderSearcher.c -o orderSearcher -lpthread -lm

clean:
	rm orderSearcher
