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
