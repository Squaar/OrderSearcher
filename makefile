all: orderSearcher

orderSearcher:
	gcc -Wall orderSearcher.c -o orderSearcher

clean:
	rm orderSearcher
