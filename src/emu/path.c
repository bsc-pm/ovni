/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "path.h"

#include "common.h"
#include <string.h>

int
path_has_prefix(const char *path, const char *prefix)
{
	if (strncmp(path, prefix, strlen(prefix)) != 0)
		return 0;

	return 1;
}

/* Counts occurences of c in path */
int
path_count(const char *path, char c)
{
	int n = 0;

	for (int i = 0; path[i] != '\0'; i++) {
		if (path[i] == c)
			n++;
	}

	return n;
}

/* Given the "a/b/c" path and '/' as sep, returns "b/c" */
int
path_next(const char *path, char sep, const char (**next))
{
	const char *p = strchr(path, sep);
	if (p == NULL) {
		err("missing '%c' in path %s", sep, path);
		return -1;
	}

	p++; /* Skip sep */
	*next = p;

	return 0;
}

/* Removes n components from the beginning.
 *
 * Examples:
 *
 *   path="a/b/c/d" and n=2 -> "c/d"
 *   path="a/b/c/d" and n=3 -> "d"
 */
int
path_strip(const char *path, int n, const char (**next))
{
	const char *p = path;
	for (; n>0; n--) {
		const char *q;
		if (path_next(p, '/', &q) != 0) {
			err("missing %d '/' in path %s", n, path);
			return -1;
		}
		p = q;
	}

	*next = p;

	return 0;
}

/* Given the "a/b/c" path 2 as n, trims the path as "a/b" */
int
path_keep(char *path, int n)
{
	for (int i = 0; path[i] != '\0'; i++) {
		if (path[i] == '/')
			n--;

		if (n == 0) {
			path[i] = '\0';
			return 0;
		}
	}

	/* if we ask path=a/b n=2, is ok */
	if (n == 1)
		return 0;

	err("missing %d components", n);
	return -1;
}

void
path_remove_trailing(char *path)
{
	int n = (int) strlen(path);
	for (int i = n - 1; i >= 0 && path[i] == '/'; i--) {
		path[i] = '\0';
	}
}

const char *
path_filename(const char *path)
{
	const char *start = strrchr(path, '/');
	if (start == NULL)
		start = path;
	else
		start++;

	return start;
}

int
path_append(char dst[PATH_MAX], const char *src, const char *extra)
{
	if (snprintf(dst, PATH_MAX, "%s/%s", src, extra) >= PATH_MAX) {
		err("path too long: %s/%s", src, extra);
		return -1;
	}

	return 0;
}

/** Copy the path src into dst. */
int
path_copy(char dst[PATH_MAX], const char *src)
{
	if (snprintf(dst, PATH_MAX, "%s", src) >= PATH_MAX) {
		err("path too long: %s", src);
		return -1;
	}

	return 0;
}

/** Strip last component from path */
void
path_dirname(char path[PATH_MAX])
{
	path_remove_trailing(path);
	int n = (int) strlen(path);
	int i;
	for (i = n - 1; i >= 0; i--) {
		if (path[i] == '/') {
			break;
		}
	}
	/* Remove all '/' */
	for (; i >= 0; i--) {
		if (path[i] != '/')
			break;
		else
			path[i] = '\0';
	}
}
