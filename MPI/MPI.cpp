#include <stdio.h>
#include <mpi.h>
#include <algorithm>
#include <ctime>

#define MAIN_RANK 0
#define MAIN_TAG 1
#define WORKER_TAG 2
#define MATRIX_SIZE 2500
#define NOT_ENOUGH_PROCESSES_NUM_ERROR 1

MPI_Status status;

int a[MATRIX_SIZE][MATRIX_SIZE];
int b[MATRIX_SIZE][MATRIX_SIZE];
int c[MATRIX_SIZE][MATRIX_SIZE]; // result matrix

template<int rows, int cols>
void FillMatrix(int(&matrix)[rows][cols]) {
	for (int i = 0; i < cols; i++) {
		for (int j = 0; j < rows; j++)
			matrix[i][j] = rand() % 9 + 1;
	}
}

template<int rows, int cols>
void PrintMatrix(int(&matrix)[rows][cols]) {
	printf("\n");
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++)
			printf("%d ", matrix[i][j]);
		printf("\n");
	}
}

int main(int argc, char* argv[])
{
	int communicator_size;
	int process_rank;
	int process_id;
	int offset;
	int rows_num;
	int workers_num;
	int remainder;
	int whole_part;
	int message_tag;
	int i;
	int j;
	int k;


	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &communicator_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

	if (communicator_size < 2)
		MPI_Abort(MPI_COMM_WORLD, NOT_ENOUGH_PROCESSES_NUM_ERROR);

	if (process_rank == MAIN_RANK) {
		FillMatrix(a);
		FillMatrix(b);
		//PrintMatrix(a);
		//PrintMatrix(b);

		long long int start = clock();

		workers_num = communicator_size - 1;
		whole_part = MATRIX_SIZE / workers_num;
		remainder = MATRIX_SIZE % workers_num;
		offset = 0;

		message_tag = MAIN_TAG;
		for (process_id = 1; process_id <= workers_num; process_id++) {
			rows_num = process_id <= remainder ? whole_part + 1 : whole_part;
			MPI_Send(&offset, 1, MPI_INT, process_id, message_tag, MPI_COMM_WORLD);
			MPI_Send(&rows_num, 1, MPI_INT, process_id, message_tag, MPI_COMM_WORLD);
			MPI_Send(&a[offset][0], rows_num * MATRIX_SIZE, MPI_INT, process_id, message_tag, MPI_COMM_WORLD);
			MPI_Send(&b, MATRIX_SIZE * MATRIX_SIZE, MPI_INT, process_id, message_tag, MPI_COMM_WORLD);
			offset += rows_num;
		}

		message_tag = WORKER_TAG;
		for (process_id = 1; process_id <= workers_num; process_id++) {
			MPI_Recv(&offset, 1, MPI_INT, process_id, message_tag, MPI_COMM_WORLD, &status);
			MPI_Recv(&rows_num, 1, MPI_INT, process_id, message_tag, MPI_COMM_WORLD, &status);
			MPI_Recv(&c[offset][0], rows_num * MATRIX_SIZE, MPI_INT, process_id, message_tag, MPI_COMM_WORLD, &status);
		}
		//PrintMatrix(c);
		long long int end = clock();
		double diff = (double)((end - start) / CLK_TCK);

		printf("%dx%d - %f sec\n", MATRIX_SIZE, MATRIX_SIZE, diff);
	}

	if (process_rank != MAIN_RANK) {
		message_tag = MAIN_TAG;
		MPI_Recv(&offset, 1, MPI_INT, MAIN_RANK, message_tag, MPI_COMM_WORLD, &status);
		MPI_Recv(&rows_num, 1, MPI_INT, MAIN_RANK, message_tag, MPI_COMM_WORLD, &status);
		MPI_Recv(&a, rows_num * MATRIX_SIZE, MPI_INT, MAIN_RANK, message_tag, MPI_COMM_WORLD, &status);
		MPI_Recv(&b, MATRIX_SIZE * MATRIX_SIZE, MPI_INT, MAIN_RANK, message_tag, MPI_COMM_WORLD, &status);

		for (k = 0; k < MATRIX_SIZE; k++) {
			for (i = 0; i < rows_num; i++) {
				c[i][k] = 0;
				for (j = 0; j < MATRIX_SIZE; j++)
					c[i][k] += a[i][j] * b[j][k];
			}
		}

		message_tag = WORKER_TAG;
		MPI_Send(&offset, 1, MPI_INT, MAIN_RANK, message_tag, MPI_COMM_WORLD);
		MPI_Send(&rows_num, 1, MPI_INT, MAIN_RANK, message_tag, MPI_COMM_WORLD);
		MPI_Send(&c, rows_num * MATRIX_SIZE, MPI_INT, MAIN_RANK, message_tag, MPI_COMM_WORLD);
	}
	MPI_Finalize();
	return 0;
}