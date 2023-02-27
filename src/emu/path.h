/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PATH_H
#define PATH_H

#include "common.h"

USE_RET int path_has_prefix(const char *path, const char *prefix);
USE_RET int path_count(const char *path, char c);
USE_RET int path_next(const char *path, char sep, const char (**next));
USE_RET int path_keep(char *path, int n);
USE_RET int path_strip(const char *path, int n, const char (**next));
        void path_remove_trailing(char *path);
USE_RET const char *path_filename(const char *path);

#endif /* PATH_H */
