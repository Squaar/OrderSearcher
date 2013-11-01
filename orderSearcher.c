//Matt Dumford - mdumford
//mdumfo2@uic.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

void *thread(void *arg);

int main(int argc, char **argv){
	if(argc != 3){
		printf("Invalid number of args.\n");
		exit(-1);
	}

	char *file = argv[1];
	int numThreads = strtol(argv[2], 0, 10);

	if(numThreads < 1){
		printf("Must have at least one thread.");
		exit(-1);
	}

	//open file
	FILE *fd = fopen(file, "rb");
	if(fd == NULL){
		perror("Error opening file");
		exit(-1);
	}

	//get size of file
	fseek(fd, 0L, SEEK_END);
	long size = ftell(fd);
	fseek(fd, 0L, SEEK_SET);

	//read in from file
	char *inBuffer = malloc(size);
	size_t readBytes = fread(inBuffer, 1, size, fd);
	if(readBytes != size){
		printf("Error reading from file");
		exit(-1);
	}

	//close file
	fclose(fd);

	int i;

	//print data
	/*printf("%s\n", inBuffer);
	for(i=0; i<size; i++){
		printf("0x%X\n", inBuffer[i]);
	}*/

	//create threads
	pthread_t threads[numThreads];
	for(i=0; i<numThreads; i++){
		char arg[size/numThreads +1];
	    memcpy(arg, &inBuffer[i*size/numThreads], size/numThreads);
		arg[size/numThreads] = '\0';
		//printf("%s\n=========================================================================\n", arg);
		if((pthread_create(&threads[i], NULL, &thread, (void *) arg)) != 0){
			perror("Error createing thread");
			exit(-1);
		}
	}

	//clean up threads
	for(i=0; i<numThreads; i++){
		pthread_join(threads[i], NULL);
	}

	return 0; 
}

void *thread(void *arg){
	printf("%s\n", (char *) arg);
	printf("================================================================================\n");
	return arg;
}
