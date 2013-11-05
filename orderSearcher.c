//Matt Dumford - mdumford
//mdumfo2@uic.edu

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
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
void wait(int sem, int semID, int nBuffers);
void signal(int sem, int semID, int nBuffers);

struct threadArgs{
	int semID;
	int threadID;
	size_t nargs;
	void *arg;
};

int range,			// The range, defined as the difference between the largest and smallest value in the data set.
	maxAbsChange,	// Maximum absolute value of change from one value to the next
	sumAbsChange;	// The sum of ( absolute value of ( change from one value to the next ))
double stdDev,		// Standard deviation --> sqrt(avg((xi-AVG)^2))
	stdDevChange;	// Standard deviation of the change between each element

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
	fseek(fd, 0L, SEEK_END);
	long size = ftell(fd);
	fseek(fd, 0L, SEEK_SET);

	//read in from file
	char *inBuffer = malloc(size);
	size_t readBytes = fread(inBuffer, 1, size, fd);
	if(readBytes != size){
		printf("Error reading from file\n");
		exit(-1);
	}

	//close file
	fclose(fd);

	int i;

	//create path of worker executable for ftok
	char workerPath[1024];
	getcwd(workerPath, sizeof(workerPath));
	strcat(workerPath, "/worker");

	//create semaphores
	int semid;
	if((semid = semget(ftok(workerPath, 'O'), NUM_GLOBALS, 00644|IPC_CREAT)) == -1){
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
	char args[numThreads][size/numThreads];

	for(i=0; i<numThreads; i++){
		argsArr[i].semID = semid;
		argsArr[i].threadID = i;

	    memcpy(args[i], &inBuffer[i*size/numThreads], size/numThreads);
	    
		argsArr[i].nargs = size/numThreads;
		argsArr[i].arg = args[i];

		//printf("%s\n=========================================================================\n", arg);
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
	
	//remove semaphores
	if(semctl(semid, 0, IPC_RMID) == -1){
		perror("Error removing semaphores ");
		exit(-1);
	}

	return 0; 
}

void *thread(void *arg){
	//printf("%s\n================================================================================\n", (char *) arg);

	struct threadArgs args = *((struct threadArgs*) arg); //convert back to struct
	int *ints = (int *) args.arg; //array of ints for math

	int i;
	double sum = 0;
	double average = 0;

	for(i=0; i<args.nargs; i++){
		sum += ints[i];
	}

	average = sum/args.nargs;

	printf("Thread: %i\n\tAverage: %f\n", args.threadID, average);

	pthread_exit(arg);
}



void wait(int sem, int semID, int nBuffers){
	struct sembuf sembuffer;
	sembuffer.sem_num = sem;
	sembuffer.sem_op = -1;
	sembuffer.sem_flg = SEM_UNDO;

	if(semop(semID, &sembuffer, 1) == -1){
		perror("Error in semop");
		exit(-1);
	}
}

void signal(int sem, int semID, int nBuffers){
	struct sembuf sembuffer;
	sembuffer.sem_num = sem;
	sembuffer.sem_op = 1;
	sembuffer.sem_flg = SEM_UNDO;

	if(semop(semID, &sembuffer, 1) == -1){
		perror("Error in semop");
		exit(-1);
	}
}
