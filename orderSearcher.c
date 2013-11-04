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

	//print data
	/*printf("%s\n", inBuffer);
	for(i=0; i<size; i++){
		printf("0x%X\n", inBuffer[i]);
	}*/

	//create threads
	pthread_t threads[numThreads];
	for(i=0; i<numThreads; i++){
		struct threadArgs args;
		args.semID = semid;
		args.threadID = i;
		char arg[size/numThreads +1];
	    memcpy(arg, &inBuffer[i*size/numThreads], size/numThreads);
		arg[size/numThreads] = '\0';
		//printf("%s\n=========================================================================\n", arg);
		if((pthread_create(&threads[i], NULL, thread, (void *) &args)) != 0){ //ALWAYS THE LAST ONE?
			perror("Error createing thread");
			exit(-1);
		}
	}

	char *rets[size/numThreads+1];

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

	struct threadArgs args = *((struct threadArgs*) arg);

	int i;
	double sum = 0;
	double average;
	int *ints = (int *) arg;

	for(i=0; (char) ints[i] != '\0'; i++){
		sum += ints[i];
	}

	int num = i;

	average = sum/num;

	printf("Thread: %i, Average: %f\n", args.threadID, average);

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
