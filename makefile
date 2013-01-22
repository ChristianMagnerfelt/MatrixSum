all: matrix_sum.c
	gcc -o MatrixSum matrix_sum.c -lpthread
clean:
	rm MatrixSum
