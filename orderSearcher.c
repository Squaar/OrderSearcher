//Matt Dumford - mdumford
//mdumfo2@uic.edu

#include <stdio.h>
#include <stdlib.h>


int main(int argc, char **argv){
	if(argc != 3){
		printf("Invalid number of args.\n");
		exit(-1);
	}

	char *file = argv[1];
	int numThreads = strtol(argv[2], 0, 10);

	printf("File: %s, numThreads: %i\n", file, numThreads);



	return 0; 
}