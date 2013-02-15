/*	description: matrix summation using pthreads
	
	author: Christian Magnerfelt
	
	email: magnerf@kth.se
   
	features:	each worker is given a stripe of the matrix where each worker computes
				its total sum, min/max value and position which is later updated to global 
				variables using a lock.
				Main prints the total sum, the max value/position 
				and the min value/position to the standard output.
             
	usage under Linux:
		./MatrixSum {size of matrix} {number of workers}
		
	building the executable:
		make debug for debug build which is equivalent to:
			gcc -Wall -O0 -g -o MatrixSum matrix_sum.c -lpthread
		make release for release build which is equivalent to:
			gcc -Wall -O2 -o MatrixSum matrix_sum.c -lpthread
*/
#ifndef _REENTRANT 
#define _REENTRANT 
#endif 
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */

/* #define DEBUG*/ /* Enable debugging */

long long g_sum = 0;	/* matrix total sum */
int g_min = INT_MAX;	/* matrix minimum value */ 
int g_minX = -1;		/* matrix minimum value x position */
int g_minY = -1;		/* matrix minimum value y position */
int g_max = INT_MIN;	/* matrix maximum value */	
int g_maxX = -1;		/* matrix maximum value x position */
int g_maxY = -1;		/* matrix maximum value y position */

pthread_mutex_t dataLock;	/* lock for shared data */

int g_count = 0;
int g_size;
pthread_mutex_t countLock;

int numWorkers;		/* number of workers */

double startTime, endTime;		/* start and end times */
int size, stripSize;			/* assume size is multiple of numWorkers */
int matrix[MAXSIZE][MAXSIZE];	/* matrix */
 
void *Worker(void *);
double readTimer();

/* read command line, initialize, and create threads */
int main(int argc, char *argv[])
{
	int i, j;
	long l;		/* use long in case of a 64-bit system */
	pthread_attr_t attr;
	pthread_t workerid[MAXWORKERS];

	/* set global thread attributes */
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	 
	/* initialize mutex */
	pthread_mutex_init(&dataLock, NULL);
	pthread_mutex_init(&countLock, NULL);
	
	/* read command line args if any */
	g_size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
	numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
	if (g_size > MAXSIZE) g_size = MAXSIZE;
	if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;

	/* initialize the matrix */
	for (i = 0; i < g_size; i++)
	{
		for (j = 0; j < g_size; j++)
		{
			matrix[i][j] = 1; /* rand()%99;*/
		}
	}
	
	/* print the matrix */
	#ifdef DEBUG
	/*for (i = 0; i < g_size; i++)
	{
		printf("[ ");
		for (j = 0; j < g_size; j++)
		{
	    	printf(" %d", matrix[i][j]);
	    }
		printf(" ]\n");
	}*/
	#endif

	/* do the parallel work: create the workers */
	startTime = readTimer();
	for (l = 0; l < numWorkers; l++)
	{
		pthread_create(&workerid[l], &attr, Worker, (void *) l);
	}
	
	/* join all worker threads*/
	for (l = 0; l < numWorkers; l++)
	{
		void * status;
		int rc = pthread_join(workerid[l], &status);
		if(rc)
		{
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
		#ifdef DEBUG
      	printf("Main: completed join with worker %ld (pthread id %lu) having a status of %ld\n", 
      		l, workerid[l], (long)status);
      	#endif		
	}
	
	/* Destroy mutexes which are not needed anymore */
	pthread_mutex_destroy(&dataLock);
	pthread_mutex_destroy(&countLock);	

	/* print results */
	printf("The total is %llu\n", g_sum);
	printf("The min element is %d at (%d , %d)\n", g_min, g_minX, g_minY);
	printf("The max element is %d at (%d , %d)\n", g_max, g_maxX, g_maxY);
	printf("%d number of rows processed\n", g_count);	
		
	/* get end time */
	endTime = readTimer();
	printf("The execution time is %g sec\n", endTime - startTime);
	return 0;
}

/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg)
{
	long myid = (long) arg;
	int total, min, minX, minY, max, maxX, maxY, i, row;
	bool done = false;
	
	#ifdef DEBUG
		printf("worker %ld (pthread id %lu) has started\n", myid, pthread_self());
	#endif
	
	while(!done)
	{
		pthread_mutex_lock(&countLock);
		if(g_count < g_size)
		{
			row = g_count; 
			g_count++;
			#ifdef DEBUG
			printf("worker %ld (pthread id %lu) working on row %d\n", myid, pthread_self(), row);
			#endif
		}
		else
		{
			done = true;
		}
		pthread_mutex_unlock(&countLock);
		
		if(done)
			break;
		
		/* sum values in my row */
		total = 0;
		min = INT_MAX;
		max = INT_MIN;
		for (i = 0; i < g_size; i++)
		{
			if(matrix[row][i] < min)
			{
				min = matrix[row][i];
				minX = i;
				minY = row;
			}
			else if(matrix[row][i] > max)
			{
				max = matrix[row][i];
				maxX = i;
				maxY = row;
			}
	  		total += matrix[row][i];
	  	}
		pthread_mutex_lock(&dataLock);

		/* save max and min value for this worker and their respective position */
		if(min < g_min)
		{
			g_min = min;
			g_minX = minX;
			g_minY = minY;
		}
		if(max > g_max)
		{
			g_max = max;
			g_maxX = maxX;
			g_maxY = maxY;	
		}
		/* add partial sum to global sum */
		g_sum += total;

		pthread_mutex_unlock(&dataLock);
	}
	pthread_exit(0);
}
/* timer */
double readTimer()
{
	static bool initialized = false;
	static struct timeval start;
	struct timeval end;
	if( !initialized )
	{
		gettimeofday( &start, NULL );
		initialized = true;
	}
	gettimeofday( &end, NULL );
	return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}
