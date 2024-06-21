/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: MIT */

#include "common.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

char *progname = NULL;
int is_debug_enabled = 0;

void
progname_set(char *name)
{
	progname = name;
}

const char *
progname_get(void)
{
	return progname;
}

void
enable_debug(void)
{
	is_debug_enabled = 1;
}

static void
vaerr(const char *prefix, const char *func, const char *errstr, va_list ap)
{
	if (progname != NULL)
		fprintf(stderr, "%s: ", progname);

	if (prefix != NULL)
		fprintf(stderr, "%s: ", prefix);

	if (func != NULL)
		fprintf(stderr, "%s: ", func);

	vfprintf(stderr, errstr, ap);

	int len = strlen(errstr);

	if (len > 0) {
		char last = errstr[len - 1];
		if (last == ':')
			fprintf(stderr, " %s\n", strerror(errno));
		else if (last != '\n' && last != '\r')
			fprintf(stderr, "\n");
	}
}

void __attribute__((format(printf, 3, 4)))
verr(const char *prefix, const char *func, const char *errstr, ...)
{
	va_list ap;
	va_start(ap, errstr);
	vaerr(prefix, func, errstr, ap);
	va_end(ap);
}

void __attribute__((format(printf, 3, 4)))
vdie(const char *prefix, const char *func, const char *errstr, ...)
{
	va_list ap;
	va_start(ap, errstr);
	vaerr(prefix, func, errstr, ap);
	va_end(ap);
	abort();
}

static int
mkdir_if_need(const char *path, mode_t mode)
{
	errno = 0;
	if (mkdir(path, mode) == 0)
		return 0;

	if (errno == EEXIST) {
		struct stat st;
		if (stat(path, &st) != 0)
			return -1;

		if (S_ISDIR(st.st_mode))
			return 0;

		errno = ENOTDIR;
		return -1;
	}

	return -1;
}

int
mkpath(const char *path, mode_t mode, int is_dir)
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
			status = mkdir_if_need(copypath, mode);
			*sp = '/';
		}
		pp = sp + 1;
	}

	/* Create last component if it is a directory */
	if (is_dir && status == 0)
		status = mkdir_if_need(copypath, mode);

	free(copypath);
	return status;
}

