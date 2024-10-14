#include <string.h>
#include <stdio.h>
#include <mpi.h>

#define MAX_NAME_SIZE 42
#define MAX_MSG_SIZE 100

int main(int argc, char *argv[])
{
    char msg[MAX_MSG_SIZE], name[MAX_NAME_SIZE];
    int rank, size, p, len;

    /* no MPI calls before this point */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
      printf("Hello from process %d of %d\n", rank, size);
      for (p = 1; p < size; p++) {
	MPI_Recv(msg, MAX_MSG_SIZE, MPI_CHAR, p, 0, MPI_COMM_WORLD,
		 MPI_STATUS_IGNORE);
	printf("%s\n", msg);
      }
    } else {
      MPI_Get_processor_name(name, &len);
      sprintf(msg, "Message from process %d at %s", rank, name);
      MPI_Send(msg, strlen(msg)+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    /* no MPI calls after this point */
    return 0;
}
