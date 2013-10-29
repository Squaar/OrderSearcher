all: orderSearcher

orderSearcher: orderSearcher.c
	gcc -Wall orderSearcher.c -o orderSearcher

clean:
	rm orderSearcher
