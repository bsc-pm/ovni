/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <errno.h>
#include <math.h>
#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "common.h"
#include "ovni.h"

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
	char outpath[PATH_MAX];
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
	fprintf(stderr, "%s: clock synchronization utility\n", progname_get());
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: %s [-o outfile] [-d ndrift_samples] [-v] [-n nsamples] [-w drift_delay]\n",
			progname_get());
	exit(EXIT_FAILURE);
}

static void
parse_options(struct options *options, int argc, char *argv[])
{
	char *filename = "clock-offsets.txt";
	char *tracedir = getenv("OVNI_TRACEDIR");
	if (tracedir == NULL)
		tracedir = OVNI_TRACEDIR;

	/* Default options */
	options->ndrift_samples = 1;
	options->nsamples = 100;
	options->verbose = 0;
	options->drift_wait = 5;

	if (snprintf(options->outpath, PATH_MAX, "%s/%s", tracedir, filename) >= PATH_MAX) {
		fprintf(stderr, "output path too long: %s/%s\n", tracedir, filename);
		exit(EXIT_FAILURE);
	}

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
				if (strlen(optarg) >= PATH_MAX - 1) {
					fprintf(stderr, "output path too long: %s\n", optarg);
					exit(EXIT_FAILURE);
				}
				strcpy(options->outpath, optarg);
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
	double *delta = malloc(sizeof(double) * (size_t) nsamples);

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

	qsort(delta, (size_t) nsamples, sizeof(double), cmp_double);

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
	return sizeof(struct offset) + sizeof(double) * (size_t) nsamples;
}

static struct offset *
table_get_offset(struct offset_table *table, int i, int nsamples)
{
	char *p = (char *) table->_offset;
	p += (size_t) i * offset_size(nsamples);

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

		table->_offset = calloc((size_t) table->nprocs, offset_size(nsamples));

		if (table->_offset == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		table->offset = malloc(sizeof(struct offset *) * (size_t) table->nprocs);

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
	MPI_Gather(sendbuf, (int) offset_size(nsamples), MPI_CHAR,
			offset, (int) offset_size(nsamples), MPI_CHAR,
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

		/* Skip the entry if hostname was repeated */
		int skip = 0;
		for (int j = 0; j < i; j++) {
			struct offset *b = table->offset[j];
			if (strcmp(offset->hostname, b->hostname) == 0) {
				skip = 1;
				break;
			}
		}

		if (skip)
			continue;

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
		if (mkpath(options->outpath, 0755, /* not subdir */ 0) != 0) {
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
			sleep((unsigned) options->drift_wait);
	}

	if (rank == 0)
		fclose(out);
}

int
main(int argc, char *argv[])
{
	progname_set("ovnisync");

	MPI_Init(&argc, &argv);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	struct options options;
	parse_options(&options, argc, argv);

	do_work(&options, rank);

	MPI_Finalize();

	return 0;
}
