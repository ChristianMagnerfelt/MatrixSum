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

#define DEBUG /* Enable debugging */

int g_sum = 0;			/* matrix total sum */
int g_min = INT_MAX;	/* matrix minimum value */ 
int g_minX = -1;		/* matrix minimum value x position */
int g_minY = -1;		/* matrix minimum value y position */
int g_max = INT_MIN;	/* matrix maximum value */	
int g_maxX = -1;		/* matrix maximum value x position */
int g_maxY = -1;		/* matrix maximum value y position */

pthread_mutex_t lock;	/* lock for shared data */

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
	pthread_mutex_init(&lock, NULL);

	/* read command line args if any */
	size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
	numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
	if (size > MAXSIZE) size = MAXSIZE;
	if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
	
	/* Calculate the strip size */
	stripSize = size/numWorkers;

	/* initialize the matrix */
	for (i = 0; i < size; i++)
	{
		for (j = 0; j < size; j++)
		{
			matrix[i][j] = rand()%99;
		}
	}
	
	/* print the matrix */
	#ifdef DEBUG
	for (i = 0; i < size; i++)
	{
		printf("[ ");
		for (j = 0; j < size; j++)
		{
	    	printf(" %d", matrix[i][j]);
	    }
		printf(" ]\n");
	}
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

	/* print results */
	printf("The total is %d\n", g_sum);
	printf("The min element is %d at (%d , %d)\n", g_min, g_minX, g_minY);
	printf("The max element is %d at (%d , %d)\n", g_max, g_maxX, g_maxY);
		
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
	int total, min, minX, minY, max, maxX, maxY, i, j, first, last;
	
	#ifdef DEBUG
		printf("worker %ld (pthread id %lu) has started\n", myid, pthread_self());
	#endif
	
	/* determine first and last rows of my strip */
	first = myid*stripSize;
	last = (myid == numWorkers - 1) ? (size - 1) : (first + stripSize - 1);

	/* sum values in my strip */
	total = 0;
	min = INT_MAX;
	max = INT_MIN;
	for (i = first; i <= last; i++)
	{
    	for (j = 0; j < size; j++)
    	{
    		if(matrix[i][j] < min)
    		{
    			min = matrix[i][j];
    			minX = j;
    			minY = i;
    		}
    		else if(matrix[i][j] > max)
    		{
    			max = matrix[i][j];
    			maxX = j;
    			maxY = i;
    		}
      		total += matrix[i][j];
      	}
	}
	pthread_mutex_lock(&lock);
	
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

	pthread_mutex_unlock(&lock);
	
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
