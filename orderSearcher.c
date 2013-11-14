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
void printGraph(int *data, int length);
void wait(int sem, int semID);
void signal(int sem, int semID);

struct threadArgs{
	int semID;
	int threadID;
	size_t nargs;
	int *arg;
};

const int NUM_GLOBALS = 5;

//current bests
int range = 0;				//0 - The range, defined as the difference between the largest and smallest value in the data set.
int maxAbsChange = 0;		//1 - Maximum absolute value of change from one value to the next
int sumAbsChange = 0;		//2 - The sum of ( absolute value of ( change from one value to the next ))
double stdDev = 0;			//3 - Standard deviation --> sqrt(avg((xi-AVG)^2))
double stdDevChange = 0;	//4 - Standard deviation of the change between each element

//size of arrays of history of best values
int rangeArrSize = 32 * sizeof(int);					//0
int maxAbsChangeArrSize = 32 * sizeof(int);				//1
int sumAbsChangeArrSize = 32 * sizeof(int);				//2
int stdDevArrSize = 32 * sizeof(double);				//3
int stdDevChangeArrSize = 32 * sizeof(double);			//4

//arrays of history of best values
int *rangeArr; //= malloc(rangeArrSize);					//0
int *maxAbsChangeArr; //= malloc(maxAbsChangeArrSize);		//1
int *sumAbsChangeArr; //= malloc(sumAbsChangeArrSize);		//2
double *stdDevArr; //= malloc(stdDevArrSize);				//3
double *stdDevChangeArr; //= malloc(stdDevChangeArrSize);  //4

//number of elements in each array
int numRange = 0;			//0
int numMaxAbsChange = 0;	//1
int numSumAbsChange = 0;	//2
int numStdDev = 0;			//3
int numStdDevChange = 0;	//4


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

	//malloc arrays
	rangeArr = malloc(rangeArrSize);
	maxAbsChangeArr = malloc(maxAbsChangeArrSize);
	sumAbsChangeArr = malloc(sumAbsChangeArrSize);
	stdDevArr = malloc(stdDevArrSize);
	stdDevChangeArr = malloc(stdDevChangeArrSize);

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
	printf("\nMinimum Range: %i\n", range);
	printf("Minimum Change between values: %i\n", maxAbsChange);
	printf("minimum sum of changes between values: %i\n", sumAbsChange);
	printf("Minimum Standard Deviation: %f\n", stdDev);
	printf("Minimum standard deviation of changes between values: %f\n", stdDevChange);
	

	//remove semaphores
	if(semctl(semid, 0, IPC_RMID) == -1){
		perror("Error removing semaphores ");
		exit(-1);
	}

	printGraph(maxAbsChangeArr, numRange);

	//cleanup
	free(ints);
	free(rangeArr);
	free(maxAbsChangeArr);
	free(sumAbsChangeArr);
	free(stdDevArr);
	free(stdDevChangeArr);

	return 0; 
}


//thread function
void *thread(void *arg){	
	struct threadArgs args = *((struct threadArgs*) arg); //convert back to struct
	int *ints = args.arg; //array of ints for math

	int i;
	int max = ints[0];
	int min = ints[0];
	int maxChange = abs(ints[0]-ints[1]);
	int sumChange = 0;
	double sum = 0;
	double average = 0;
	double avgChange = 0;
	double devSum = 0;
	double devChangeSum = 0;

	int stdDevStill = 1;
	int sumChangeStill = 1;
	int stdDevChangeStill = 1;

	for(i=0; i<args.nargs; i++){
		sum += ints[i];

		if(i != args.nargs-1){
			//find max change between values
			if(abs(ints[i]-ints[i+1]) < maxChange)
				maxChange = abs(ints[i] - ints[i+1]);

			//compute sum of change between values
			sumChange += abs(ints[i] - ints[i+1]);
		}

		//find max and min values
		if(ints[i] > max)
			max = ints[i];
		if(ints[i] < min)
			min = ints[i];
	}

	//averages
	avgChange = sumChange/(args.nargs-1);
	average = sum/args.nargs;

	//compute range
	if(numRange == 0 || max-min < range){
		wait(0, args.semID);
		if(numRange == rangeArrSize-1){
			rangeArr = realloc(rangeArr, rangeArrSize *2);
			rangeArrSize *= 2;
		}
		range = max-min;
		rangeArr[numRange] = range;
		numRange++;

		signal(0, args.semID);
	}

	//compute maximum absolute change
	if(numMaxAbsChange == 0 || maxChange < maxAbsChange){
		wait(1, args.semID);
		if(numMaxAbsChange == maxAbsChangeArrSize-1){
			maxAbsChangeArr = realloc(maxAbsChangeArr, maxAbsChangeArrSize *2);
			maxAbsChangeArrSize *=2;
		}
		maxAbsChange = maxChange;
		maxAbsChangeArr[numMaxAbsChange] = maxAbsChange;
		numMaxAbsChange++;
		signal(1, args.semID);
	}

	//set sumAbsChange
	if(sumChangeStill && (numSumAbsChange == 0 || sumChange < sumAbsChange)){
		wait(2, args.semID);
		if(numSumAbsChange == sumAbsChangeArrSize-1){
			sumAbsChangeArr = realloc(sumAbsChangeArr, sumAbsChangeArrSize *2);
			sumAbsChangeArrSize *=2;
		}
		sumAbsChange = sumChange;
		sumAbsChangeArr[numSumAbsChange] = sumAbsChange;
		numSumAbsChange++;
		signal(2, args.semID);
	}

	//compute standard deviations
	for(i=0; i<args.nargs; i++){
		if(stdDevStill){
			devSum += pow(ints[i]-average, 2);
			if(stdDev !=0 && devSum > pow(stdDev, 2) * args.nargs)
				stdDevStill = 0;
		}
		if(stdDevChangeStill){
			if(i != args.nargs-1)
				devChangeSum += pow(abs(ints[i]-ints[i+1]) - avgChange, 2);
			if(stdDevChange !=0 && devChangeSum > pow(stdDevChange, 2) * args.nargs)
				stdDevChangeStill = 0;
		}
		
	}

	//set standard deviation
	if(stdDevStill && (numStdDev == 0 || stdDev > sqrt(devSum/args.nargs))){
		wait(3, args.semID);
		if(numStdDev == stdDevArrSize-1){
			stdDevArr = realloc(stdDevArr, stdDevArrSize*2);
			stdDevArrSize *=2;
		}
		stdDev = sqrt(devSum/args.nargs);
		stdDevArr[numStdDev] = stdDev;
		numStdDev++;
		signal(3, args.semID);
	}

	//set stanard deviation of changes between values
	if(stdDevChangeStill && (numStdDevChange == 0 || stdDevChange > sqrt(devChangeSum/(args.nargs-1)))){
		wait(4, args.semID);
		if(numStdDevChange == stdDevChangeArrSize-1){
			stdDevChangeArr = realloc(stdDevChangeArr, stdDevChangeArrSize *2);
			stdDevChangeArrSize *=2;
		}
		stdDevChange = sqrt(devChangeSum/(args.nargs-1));
		stdDevChangeArr[numStdDevChange] = stdDevChange;
		numStdDevChange++;
		signal(4, args.semID);
	}

	pthread_exit(arg);
}

void printGraph(int *data, int length){
	const int WIDTH = length;
	const int HEIGHT = 55;
	char graph[HEIGHT][WIDTH];
	int i,j;

	for(i=0; i<HEIGHT; i++)
		for(j=0; j<WIDTH; j++)
			graph[i][j] = ' ';

	int max = data[0];
	int min = data[0];

	for(i=0; i<length; i++){
		if(data[i] > max)
			max = data[i];
		if(data[i] < min)
			min = data[i];
	}

	int range = max - min;
	if(range == 0)
		range = 1;

	int newVals[length];
	for(i=0; i<length; i++){
		newVals[i] = (((data[i] - min) * HEIGHT) / range);
		graph[newVals[i]][i] = 'X';
	}


	printf("%i\n", max);
	for(i=0; i<HEIGHT; i++){
		printf("|");
		for(j=0; j<WIDTH; j++){
			printf("%c", graph[i][j]);
		}
		printf("\n");
	}
	printf("+");
	for(i=0; i<WIDTH; i++)
		printf("-");
	printf("\n%i\n", min);

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
