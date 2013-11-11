//Matt Dumford - mdumford
//mdumfo2@uic.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <sys/sem.h>

#if defined(_GNU_LIBRARY_) && !defined(_SEM_SEMUN_UNDEFINED_)
#else
union semun{
        int val;
        struct semid_ds *buf;
        unsigned short *array;
        struct seminfo *_buf;
} semArg;
#endif

void *thread(void *arg);
void wait(int sem, int semID);
void signal(int sem, int semID);

struct threadArgs{
	int semID;
	int threadID;
	size_t nargs;
	int *arg;
};

int range = 0;				// The range, defined as the difference between the largest and smallest value in the data set.
int maxAbsChange = 0;		// Maximum absolute value of change from one value to the next
int sumAbsChange = 0;		// The sum of ( absolute value of ( change from one value to the next ))
double stdDev = 0;			// Standard deviation --> sqrt(avg((xi-AVG)^2))
double stdDevChange = 0;	// Standard deviation of the change between each element


const int NUM_GLOBALS = 5;


int main(int argc, char **argv){
	if(argc != 3){
		printf("Invalid number of args.\n");
		exit(-1);
	}

	char *file = argv[1];
	int numThreads = strtol(argv[2], 0, 10);

	if(numThreads < 1){
		printf("Must have at least one thread.\n");
		exit(-1);
	}

	printf("Matt Dumford - mdumford\nmdumfo2@uic.edu\n\n");

	//open file
	FILE *fd = fopen(file, "rb");
	if(fd == NULL){
		perror("Error opening file");
		exit(-1);
	}

	//get size of file
	if(fseek(fd, 0L, SEEK_END) != 0){
		printf("Error Seeking");
		exit(-1);
	}
	long size = 0;
	if((size = ftell(fd)) == -1L){
		printf("Error Telling");
		exit(-1);
	}
	if(fseek(fd, 0L, SEEK_SET) != 0){
		printf("Error Seeking");
		exit(-1);
	}

	//read in from file
	char *inBuffer = malloc(size * sizeof(char));
	size_t readBytes = 0;
	readBytes = fread(inBuffer, size, 1, fd);
	if(readBytes != 1){
		printf("Error reading from file\n");
		exit(-1);
	}

	int i=0;

	//convert to ints
	int *ints = malloc(size * sizeof(int));
	for(i=0; i<size; i++){
		ints[i] = inBuffer[i];
	}

	free(inBuffer);

	//close file
	fclose(fd);

	//create path of orderSearcher.c executable for ftok
	char path[1024];
	getcwd(path, sizeof(path));
	strcat(path, "/orderSearcher.c");

	//create semaphores
	int semid;
	if((semid = semget(ftok(path, 'O'), NUM_GLOBALS, 00644|IPC_CREAT)) == -1){
		perror("Error creating semaphores ");
		exit(-1);
	}

	semArg.val = 1;

	//initialize semaphores to 1
	for(i=0; i<NUM_GLOBALS; i++){
		if(semctl(semid, i, SETVAL, semArg) == -1){
			perror("Error initializing semaphore ");
			exit(-1);
		}
	}

	//create threads
	pthread_t threads[numThreads];
	struct threadArgs argsArr[numThreads];

	for(i=0; i<numThreads; i++){
		argsArr[i].semID = semid;
		argsArr[i].threadID = i;
		argsArr[i].nargs = size/numThreads;
		argsArr[i].arg = &ints[i*size/numThreads];

		if((pthread_create(&threads[i], NULL, thread, (void *) &argsArr[i])) != 0){
			perror("Error createing thread");
			exit(-1);
		}
	}

	void *rets[numThreads];

	//clean up threads
	for(i=0; i<numThreads; i++){
		pthread_join(threads[i], (void **) &rets[i]);
		//printf("%s\n", (char *) rets[i]);
	}

	//print best results
	printf("\nBest Standard Deviation: %f\n", stdDev);

	//remove semaphores
	if(semctl(semid, 0, IPC_RMID) == -1){
		perror("Error removing semaphores ");
		exit(-1);
	}
	free(ints);
	return 0; 
}

void *thread(void *arg){	
	struct threadArgs args = *((struct threadArgs*) arg); //convert back to struct
	int *ints = args.arg; //array of ints for math

	int i;
	double sum = 0;
	double average = 0;
	double devSum=0;

	int stdDevStill = 1;

	for(i=0; i<args.nargs; i++){
		sum += ints[i];
	}

	average = sum/args.nargs;

	if(stdDevStill){
		for(i=0; i<args.nargs; i++){
			devSum += pow(ints[i]-average, 2);
			if(stdDev !=0){ //check if this stdDev can still possibly be the best
				if(devSum > pow(stdDev, 2) * args.nargs)
					stdDevStill = 0;
			}
		}
	
		if(stdDev == 0 || stdDev > sqrt(devSum/args.nargs)){
			wait(3, args.semID);
			stdDev = sqrt(devSum/args.nargs);
			printf("stdDev: %f\n", stdDev);
			signal(3, args.semID);
		}
	}

	//stats for this thread
	//printf("Thread: %i\n\tAverage: %f\n", args.threadID, average);

	pthread_exit(arg);
}



void wait(int sem, int semID){
	struct sembuf sembuffer;
	sembuffer.sem_num = sem;
	sembuffer.sem_op = -1;
	sembuffer.sem_flg = SEM_UNDO;

	if(semop(semID, &sembuffer, 1) == -1){
		perror("Error in semop");
		exit(-1);
	}
}

void signal(int sem, int semID){
	struct sembuf sembuffer;
	sembuffer.sem_num = sem;
	sembuffer.sem_op = 1;
	sembuffer.sem_flg = SEM_UNDO;

	if(semop(semID, &sembuffer, 1) == -1){
		perror("Error in semop");
		exit(-1);
	}
}
