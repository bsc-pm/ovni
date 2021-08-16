#include <limits.h>
#include <time.h>
#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "ovni.h"

static double
get_time()
{
	struct timespec tv;
	if(clock_gettime(CLOCK_MONOTONIC, &tv) != 0)
	{
		perror("clock_gettime failed");
		exit(EXIT_FAILURE);
	}
	return (double)(tv.tv_sec) * 1.0e-9 +
		(double)tv.tv_nsec;
}

static int
cmp_double(const void *pa, const void *pb)
{
	double a, b;

	a = *(const double *) pa;
	b = *(const double *) pb;

	if(a < b)
		return -1;
	else if(a > b)
		return 1;
	else
		return 0;
}

/* Called by rank 0 */
static void
get_offset(double *timetable, char (*hosttable)[OVNI_MAX_HOSTNAME], int nproc, int nsamples)
{
	int i, j;
	double median, mean, var, std;
	double *offset;
	double *delta;

	offset = malloc(nproc * sizeof(double));
	delta = malloc(nsamples * sizeof(double));

	if(!offset || !delta)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	/* We use as ref the clock in rank 0 */

	printf("%-10s %-20s %-20s %-20s\n", "rank", "hostname", "offset_median", "offset_std");
	for(i=0; i<nproc; i++)
	{
		for(j=0; j<nsamples; j++)
			delta[j] = timetable[j] - timetable[i*nsamples + j];

		mean = 0.0;
		var = 0.0;

		qsort(delta, nsamples, sizeof(*delta), cmp_double);

		median = delta[nsamples / 2];

		for(j=0; j<nsamples; j++)
		{
			//printf("%f\n", delta[j]);
			mean += delta[j];
		}

		mean /= nsamples;
		for(j=0; j<nsamples; j++)
		{
			var += (delta[j] - mean) * (delta[j] - mean);
		}
		var /= (double) (nsamples - 1);
		std = sqrt(var);
		offset[i] = mean;
		printf("%-10d %-20s %-20.0f %-20f\n", i, hosttable[i], median, std);
	}

	free(offset);
	free(delta);
}

int
main(int argc, char *argv[])
{
	double *t;
	double *timetable;
	int i, rank, nprocs, nsamples;
	char (*hosttable)[OVNI_MAX_HOSTNAME];

	MPI_Init(&argc, &argv);

	if(!argv[1])
		nsamples = 100;
	else
		nsamples = atoi(argv[1]);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	timetable = calloc(nsamples * nprocs, sizeof(double));
	hosttable = calloc(nprocs, sizeof(*hosttable));
	t = calloc(nsamples, sizeof(double));

	if(!timetable || !t || !hosttable)
	{
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	/* Warm up iterations */
	for(i=0; i<20; i++)
	{
		MPI_Barrier(MPI_COMM_WORLD);
		get_time();
	}

	for(i=0; i<nsamples; i++)
	{
		MPI_Barrier(MPI_COMM_WORLD);
		t[i] = get_time();
	}

	if(gethostname(hosttable[rank], OVNI_MAX_HOSTNAME) != 0)
	{
		perror("gethostname");
		exit(EXIT_FAILURE);
	}

	void *sendbuff = (rank > 0) ? hosttable[rank] : MPI_IN_PLACE;

	MPI_Gather(sendbuff, sizeof(*hosttable), MPI_CHAR,
		hosttable[0], sizeof(*hosttable), MPI_CHAR,
		0, MPI_COMM_WORLD);

	MPI_Gather(t, nsamples, MPI_DOUBLE,
		timetable, nsamples, MPI_DOUBLE,
		0, MPI_COMM_WORLD);

	if(rank == 0)
		get_offset(timetable, hosttable, nprocs, nsamples);

	free(hosttable);
	free(timetable);
	free(t);
	MPI_Finalize();
	return 0;
}
