/*	description: matrix summation using pthreads
	
	author: Christian Magnerfelt
	
	email: magnerf@kth.se
   
	features: uses a barrier; the Worker[0] computes
             the total sum from partial sums, finds max of maxes 
             and min of mins computed by Workers.
             Worker[0] also prints the total sum, the max value/position 
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

pthread_mutex_t barrier;  /* mutex lock for the barrier */
pthread_cond_t go;        /* condition variable for leaving */
int numWorkers;           /* number of workers */ 
int numArrived = 0;       /* number who have arrived */

/* a reusable counter barrier */
void Barrier()
{
	pthread_mutex_lock(&barrier);
	numArrived++;
	if (numArrived == numWorkers)
	{
		numArrived = 0;
		pthread_cond_broadcast(&go);
	}
	else
	{
    	pthread_cond_wait(&go, &barrier);
    }
	pthread_mutex_unlock(&barrier);
}

/* timer */
double read_timer()
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
struct Position
{
	int min_x[MAXWORKERS];
	int min_y[MAXWORKERS];
	int max_x[MAXWORKERS];
	int max_y[MAXWORKERS];
} positions;

double start_time, end_time;	/* start and end times */
int size, stripSize;			/* assume size is multiple of numWorkers */
int sums[MAXWORKERS];			/* partial sums */
int mins[MAXWORKERS];			/* minimums */
int maxes[MAXWORKERS];			/* maximums */	
int matrix[MAXSIZE][MAXSIZE];	/* matrix */
 
void *Worker(void *);

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

	/* initialize mutex and condition variable */
	pthread_mutex_init(&barrier, NULL);
	pthread_cond_init(&go, NULL);

	/* read command line args if any */
	size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
	numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
	if (size > MAXSIZE) size = MAXSIZE;
	if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
	
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
	start_time = read_timer();
	for (l = 0; l < numWorkers; l++)
	{
		pthread_create(&workerid[l], &attr, Worker, (void *) l);
	}
	pthread_exit(NULL);
}

/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg)
{
	long myid = (long) arg;
	int total, min, min_x, min_y, max, max_x, max_y, i, j, first, last;
	
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
    			min_x = j;
    			min_y = i;
    		}
    		else if(matrix[i][j] > max)
    		{
    			max = matrix[i][j];
    			max_x = j;
    			max_y = i;
    		}
      		total += matrix[i][j];
      	}
	}
	/* save max and min value for this worker */
	mins[myid] = min;
	maxes[myid] = max;
	
	/* save max and min positions of this worker */
	positions.min_x[myid] = min_x;
	positions.min_y[myid] = min_y;
	positions.max_x[myid] = max_x;
	positions.max_y[myid] = max_y;
	
	/* save partial sum of this worker */
  	sums[myid] = total;
  	
	Barrier();
	if (myid == 0)
	{
		/* Summarize all partial sums */
		total = 0;
		for (i = 0; i < numWorkers; i++)
		{
			total += sums[i];
		}
		/* find min of mins */
		min = INT_MAX;
		min_x = -1;
		min_y = -1;
		for (i = 0; i < numWorkers; i++)
		{
			if(mins[i] < min)
			{
				min = mins[i];
				min_x = positions.min_x[i];
				min_y = positions.min_y[i];
			}
		}
		/* find max of maxes */
		max = INT_MIN;
		for (i = 0; i < numWorkers; i++)
		{
			if(maxes[i] > max)
			{
				max = maxes[i];
				max_x = positions.max_x[i];
				max_y = positions.max_y[i];	
			}
		}
		/* get end time */
		end_time = read_timer();

		/* print results */
		printf("The total is %d\n", total);
		printf("The min element is %d at (%d , %d)\n", min, min_x, min_y);
		printf("The max element is %d at (%d , %d)\n", max, max_x, max_y); 
		printf("The execution time is %g sec\n", end_time - start_time);
	}
	return 0;
}
