/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PATH_H
#define PATH_H

int path_has_prefix(const char *path, const char *prefix);
int path_count(const char *path, char c);
int path_next(const char *path, char sep, const char (**next));
int path_keep(char *path, int n);
int path_strip(const char *path, int n, const char (**next));

#endif /* PATH_H */
