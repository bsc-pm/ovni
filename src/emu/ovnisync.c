/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "ovni.h"

const char progname[] = "ovnisync";

struct offset {
	/* All in nanoseconds */
	double delta_mean;
	double delta_median;
	double delta_var;
	double delta_std;

	/* The value selected for the offset */
	int64_t offset;

	/* In seconds */
	double wall_t0;
	double wall_t1;

	char hostname[OVNI_MAX_HOSTNAME];
	int rank;

	int nsamples;

	/* Flexible array */
	double clock_sample[];
};

struct offset_table {
	int nprocs;
	struct offset *_offset;
	struct offset **offset;
};

struct options {
	int nsamples;
	int ndrift_samples;
	int drift_wait; /* in seconds */
	int verbose;
	char *outpath;
};

static double
get_time(clockid_t clock, int use_ns)
{
	struct timespec tv;
	if (clock_gettime(clock, &tv) != 0) {
		perror("clock_gettime failed");
		exit(EXIT_FAILURE);
	}

	if (use_ns)
		return (double) (tv.tv_sec) * 1.0e9 + (double) tv.tv_nsec;

	return (double) (tv.tv_sec) + (double) tv.tv_nsec * 1.0e-9;
}

static int
cmp_double(const void *pa, const void *pb)
{
	double a = *(const double *) pa;
	double b = *(const double *) pb;

	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
}

static void
usage(void)
{
	fprintf(stderr, "%s: clock synchronization utility\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s [-o outfile] [-d ndrift_samples] [-v] [-n nsamples] [-w drift_delay]\n",
			progname);
	exit(EXIT_FAILURE);
}

static int
try_mkdir(const char *path, mode_t mode)
{
	struct stat st;

	if (stat(path, &st) != 0) {
		/* Directory does not exist */
		return mkdir(path, mode);
	} else if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		return -1;
	}

	return 0;
}

static int
mkpath(const char *path, mode_t mode)
{
	char *copypath = strdup(path);

	/* Remove trailing slash */
	int last = strlen(path) - 1;
	while (last > 0 && copypath[last] == '/')
		copypath[last--] = '\0';

	int status = 0;
	char *pp = copypath;
	char *sp;
	while (status == 0 && (sp = strchr(pp, '/')) != 0) {
		if (sp != pp) {
			/* Neither root nor double slash in path */
			*sp = '\0';
			status = try_mkdir(copypath, mode);
			*sp = '/';
		}
		pp = sp + 1;
	}

	free(copypath);
	return status;
}

static void
parse_options(struct options *options, int argc, char *argv[])
{
	/* Default options */
	options->ndrift_samples = 1;
	options->nsamples = 100;
	options->verbose = 0;
	options->drift_wait = 5;
	options->outpath = "ovni/clock-offsets.txt";

	int opt;
	while ((opt = getopt(argc, argv, "d:vn:w:o:h")) != -1) {
		switch (opt) {
			case 'd':
				options->ndrift_samples = atoi(optarg);
				break;
			case 'w':
				options->drift_wait = atoi(optarg);
				break;
			case 'v':
				options->verbose = 1;
				break;
			case 'n':
				options->nsamples = atoi(optarg);
				break;
			case 'o':
				options->outpath = optarg;
				break;
			case 'h':
			default: /* '?' */
				usage();
		}
	}

	if (optind < argc) {
		fprintf(stderr, "error: unexpected extra arguments\n");
		exit(EXIT_FAILURE);
	}
}

static void
get_clock_samples(struct offset *offset, int nsamples)
{
	/* Keep the wall time as well */
	offset->wall_t0 = get_time(CLOCK_REALTIME, 0);

	offset->nsamples = nsamples;

	for (int i = 0; i < nsamples; i++) {
		MPI_Barrier(MPI_COMM_WORLD);
		offset->clock_sample[i] = get_time(CLOCK_MONOTONIC, 1);
	}

	offset->wall_t1 = get_time(CLOCK_REALTIME, 0);
}

static void
fill_offset(struct offset *offset, int nsamples)
{
	/* Identify the rank */
	MPI_Comm_rank(MPI_COMM_WORLD, &offset->rank);

	/* Fill the host name */
	if (gethostname(offset->hostname, OVNI_MAX_HOSTNAME) != 0) {
		perror("gethostname");
		exit(EXIT_FAILURE);
	}

	// printf("rank=%d hostname=%s\n", offset->rank, offset->hostname);

	/* Warm up iterations */
	int warmup_nsamples = nsamples >= 20 ? 20 : nsamples;
	get_clock_samples(offset, warmup_nsamples);

	get_clock_samples(offset, nsamples);
}

static void
offset_compute_delta(struct offset *ref, struct offset *cur, int nsamples, int verbose)
{
	double *delta = malloc(sizeof(double) * nsamples);

	if (delta == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < nsamples; i++) {
		delta[i] = ref->clock_sample[i] - cur->clock_sample[i];
		if (verbose) {
			printf("rank=%d  sample=%d  delta=%f  ref=%f  cur=%f\n",
					cur->rank, i, delta[i],
					ref->clock_sample[i],
					cur->clock_sample[i]);
		}
	}

	qsort(delta, nsamples, sizeof(double), cmp_double);

	cur->delta_median = delta[nsamples / 2];
	cur->delta_mean = 0;
	for (int i = 0; i < nsamples; i++)
		cur->delta_mean += delta[i];

	cur->delta_mean /= nsamples;
	cur->delta_var = 0;
	for (int i = 0; i < nsamples; i++)
		cur->delta_var += (delta[i] - cur->delta_mean) * (delta[i] - cur->delta_mean);

	cur->delta_var /= (double) (nsamples - 1);
	cur->delta_std = sqrt(cur->delta_var);

	/* The median is the selected metric for the offset */
	cur->offset = (int64_t) cur->delta_median;

	free(delta);
}

static size_t
offset_size(int nsamples)
{
	return sizeof(struct offset) + sizeof(double) * nsamples;
}

static struct offset *
table_get_offset(struct offset_table *table, int i, int nsamples)
{
	char *p = (char *) table->_offset;
	p += i * offset_size(nsamples);

	return (struct offset *) p;
}

static struct offset_table *
build_offset_table(int nsamples, int rank, int verbose)
{
	struct offset_table *table = NULL;
	struct offset *offset = NULL;

	/* The rank 0 must build the table */
	if (rank == 0) {
		table = malloc(sizeof(*table));

		if (table == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		MPI_Comm_size(MPI_COMM_WORLD, &table->nprocs);

		table->_offset = calloc(table->nprocs, offset_size(nsamples));

		if (table->_offset == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		table->offset = malloc(sizeof(struct offset *) * table->nprocs);

		if (table->offset == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		for (int i = 0; i < table->nprocs; i++)
			table->offset[i] = table_get_offset(table, i, nsamples);

		offset = table->offset[0];
	} else {
		/* Otherwise just allocate one offset */
		offset = calloc(1, offset_size(nsamples));

		if (offset == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		table = NULL;
	}

	/* Each rank fills its own offset */
	fill_offset(offset, nsamples);

	void *sendbuf = rank == 0 ? MPI_IN_PLACE : offset;

	/* Then collect all the offsets into the rank 0 */
	MPI_Gather(sendbuf, offset_size(nsamples), MPI_CHAR,
			offset, offset_size(nsamples), MPI_CHAR,
			0, MPI_COMM_WORLD);

	/* Finish the offsets by computing the deltas on rank 0 */
	if (rank == 0) {
		for (int i = 0; i < table->nprocs; i++) {
			offset_compute_delta(offset, table->offset[i],
					nsamples, verbose);
		}
	}

	/* Cleanup for non-zero ranks */
	if (rank != 0)
		free(offset);

	return table;
}

static void
print_drift_header(FILE *out, struct offset_table *table)
{
	fprintf(out, "%-20s", "wallclock");

	for (int i = 0; i < table->nprocs; i++)
		fprintf(out, " %-20s", table->offset[i]->hostname);

	fprintf(out, "\n");
}

static void
print_drift_row(FILE *out, struct offset_table *table)
{
	fprintf(out, "%-20f", table->offset[0]->wall_t1);

	for (int i = 0; i < table->nprocs; i++)
		fprintf(out, " %-20ld", table->offset[i]->offset);

	fprintf(out, "\n");
}

static void
print_table_detailed(FILE *out, struct offset_table *table)
{
	fprintf(out, "%-10s %-20s %-20s %-20s %-20s\n",
			"rank", "hostname", "offset_median", "offset_mean", "offset_std");

	for (int i = 0; i < table->nprocs; i++) {
		struct offset *offset = table->offset[i];
		fprintf(out, "%-10d %-20s %-20ld %-20f %-20f\n",
				i, offset->hostname, offset->offset,
				offset->delta_mean, offset->delta_std);
	}
}

static void
do_work(struct options *options, int rank)
{
	FILE *out = NULL;

	int drift_mode = options->ndrift_samples > 1 ? 1 : 0;

	if (rank == 0) {
		if (mkpath(options->outpath, 0755) != 0) {
			fprintf(stderr, "mkpath(%s) failed: %s\n",
					options->outpath, strerror(errno));
			exit(EXIT_FAILURE);
		}

		out = fopen(options->outpath, "w");
		if (out == NULL) {
			fprintf(stderr, "fopen(%s) failed: %s\n",
					options->outpath, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	for (int i = 0; i < options->ndrift_samples; i++) {
		struct offset_table *table = build_offset_table(
				options->nsamples, rank, options->verbose);

		if (rank == 0) {
			if (drift_mode) {
				if (i == 0)
					print_drift_header(out, table);

				print_drift_row(out, table);
			} else {
				print_table_detailed(out, table);
			}

			free(table->_offset);
			free(table->offset);
			free(table);
		}

		if (drift_mode)
			sleep(options->drift_wait);
	}

	if (rank == 0)
		fclose(out);
}

int
main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	struct options options;
	parse_options(&options, argc, argv);

	do_work(&options, rank);

	MPI_Finalize();

	return 0;
}
