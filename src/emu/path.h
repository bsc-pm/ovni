/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PATH_H
#define PATH_H

#include <limits.h>
#include "common.h"

USE_RET int path_has_prefix(const char *path, const char *prefix);
USE_RET int path_count(const char *path, char c);
USE_RET int path_next(const char *path, char sep, const char (**next));
USE_RET int path_keep(char *path, int n);
USE_RET int path_strip(const char *path, int n, const char (**next));
        void path_remove_trailing(char *path);
USE_RET const char *path_filename(const char *path);
USE_RET int path_append(char dst[PATH_MAX], const char *src, const char *extra);
USE_RET int path_copy(char dst[PATH_MAX], const char *src);
        void path_dirname(char path[PATH_MAX]);

#endif /* PATH_H */
