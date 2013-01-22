debug: matrix_sum.c
	gcc -Wall -O0 -g -o MatrixSum matrix_sum.c -lpthread
release: matrix_sum.c
	gcc -Wall -O2 -o MatrixSum matrix_sum.c -lpthread	
clean:
	rm MatrixSum
