/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 200112L

#include "cfg.h"

#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "config.h"

static int
copy_file(const char *src, const char *dst)
{
	char buffer[1024];

	FILE *infile = fopen(src, "r");

	if (infile == NULL) {
		err("fopen(%s) failed:", src);
		return -1;
	}

	FILE *outfile = fopen(dst, "w");

	if (outfile == NULL) {
		err("fopen(%s) failed:", src);
		return -1;
	}

	size_t bytes;
	while ((bytes = fread(buffer, 1, sizeof(buffer), infile)) > 0)
		fwrite(buffer, 1, bytes, outfile);

	fclose(outfile);
	fclose(infile);

	return 0;
}

static int
copy_recursive(const char *src, const char *dst)
{
	DIR *dir;
	int failed = 0;

	if ((dir = opendir(src)) == NULL) {
		err("opendir '%s' failed:", src);
		return -1;
	}

	if (mkdir(dst, 0755) != 0) {
		err("mkdir '%s' failed:", src);
		return -1;
	}

	/* Use the heap, as the recursion may exhaust the stack */
	char *newsrc = calloc(1, PATH_MAX);
	if (newsrc == NULL) {
		err("calloc failed:");
		return -1;
	}

	char *newdst = calloc(1, PATH_MAX);
	if (newdst == NULL) {
		die("calloc failed:");
		return -1;
	}

	struct dirent *dirent;
	while ((dirent = readdir(dir)) != NULL) {
		struct stat st;

		if (strcmp(dirent->d_name, ".") == 0)
			continue;

		if (strcmp(dirent->d_name, "..") == 0)
			continue;

		int n = snprintf(newsrc, PATH_MAX, "%s/%s",
				src, dirent->d_name);

		if (n >= PATH_MAX) {
			err("src path too long: %s/%s", src, dirent->d_name);
			failed = 1;
			continue;
		}

		int m = snprintf(newdst, PATH_MAX, "%s/%s",
				dst, dirent->d_name);

		if (m >= PATH_MAX) {
			err("dst path too long: %s/%s", dst, dirent->d_name);
			failed = 1;
			continue;
		}

		if (stat(newsrc, &st) != 0) {
			err("stat '%s' failed:", newsrc);
			failed = 1;
			continue;
		}

		if (S_ISDIR(st.st_mode)) {
			if (copy_recursive(newsrc, newdst) != 0) {
				failed = 1;
			}
		} else {
			if (copy_file(newsrc, newdst) != 0) {
				failed = 1;
			}
		}
	}

	closedir(dir);

	free(newsrc);
	free(newdst);

	if (failed) {
		err("some files couldn't be copied");
		return -1;
	}

	return 0;
}

int
cfg_generate(const char *tracedir)
{
	/* TODO: generate the configuration files dynamically instead of
	 * copying them from a directory */

	/* Allow override so we can run the tests without install */
	char *src = getenv("OVNI_CONFIG_DIR");

	if (src == NULL)
		src = OVNI_CONFIG_DIR;

	char dst[PATH_MAX];
	char *cfg = "cfg";
	if (snprintf(dst, PATH_MAX, "%s/%s", tracedir, cfg) >= PATH_MAX) {
		err("path too long: %s/%s", tracedir, cfg);
		return -1;
	}

	struct stat st;
	if (stat(dst, &st) == 0) {
		err("directory '%s' already exists, skipping config copy", dst);
		return -1;
	}

	if (copy_recursive(src, dst) != 0) {
		err("cannot copy config files: recursive copy failed");
		return -1;
	}

	return 0;
}
