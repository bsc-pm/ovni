#ifndef COMMON_H
#define COMMON_H

/* Debug macros */

#ifdef ENABLE_DEBUG
# define dbg(...) fprintf(stderr, __VA_ARGS__);
#else
# define dbg(...)
#endif

#define err(...) fprintf(stderr, __VA_ARGS__);
#define die(...) do { err("fatal: " __VA_ARGS__); abort(); } while (0)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define UNUSED(x) (void)(x)

/* Poison assert */
#pragma GCC poison assert

#endif /* COMMON_H */
