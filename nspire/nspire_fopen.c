/*
 * nspire_fopen.c - Linker-level file I/O wrappers for TI-Nspire
 *
 * On TI-Nspire, ALL files must have .tns extension.
 * This uses the --wrap linker flag to intercept fopen(), open() and stat()
 * calls and automatically append ".tns" when needed.
 *
 * IMPORTANT: This file must be compiled WITHOUT -flto for --wrap to work.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>

/* Provided by the linker --wrap mechanism */
extern FILE *__real_fopen(const char *path, const char *mode);
extern int __real_open(const char *path, int flags, ...);
extern int __real_stat(const char *path, struct stat *buf);

FILE *__wrap_fopen(const char *path, const char *mode) {
    size_t len = strlen(path);
    int has_tns = (len >= 4 && strcmp(path + len - 4, ".tns") == 0);

    /* For write/append modes: always use .tns extension */
    if (!has_tns && (strchr(mode, 'w') || strchr(mode, 'a'))) {
        char tns_path[320];
        snprintf(tns_path, sizeof(tns_path), "%s.tns", path);
        return __real_fopen(tns_path, mode);
    }

    /* For read modes: try original path first, then with .tns */
    FILE *f = __real_fopen(path, mode);
    if (f) return f;

    if (!has_tns) {
        char tns_path[320];
        snprintf(tns_path, sizeof(tns_path), "%s.tns", path);
        f = __real_fopen(tns_path, mode);
    }
    return f;
}

int __wrap_open(const char *path, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }

    size_t len = strlen(path);
    int has_tns = (len >= 4 && strcmp(path + len - 4, ".tns") == 0);

    /* For write/create modes: always use .tns extension */
    if (!has_tns && (flags & (O_WRONLY | O_RDWR | O_CREAT))) {
        char tns_path[320];
        snprintf(tns_path, sizeof(tns_path), "%s.tns", path);
        return __real_open(tns_path, flags, mode);
    }

    /* For read modes: try original path first, then with .tns */
    int fd = __real_open(path, flags, mode);
    if (fd >= 0) return fd;

    if (!has_tns) {
        char tns_path[320];
        snprintf(tns_path, sizeof(tns_path), "%s.tns", path);
        fd = __real_open(tns_path, flags, mode);
    }
    return fd;
}

int __wrap_stat(const char *path, struct stat *buf) {
    size_t len = strlen(path);
    int has_tns = (len >= 4 && strcmp(path + len - 4, ".tns") == 0);

    /* Try original path first */
    int r = __real_stat(path, buf);
    if (r == 0) return 0;

    /* Try with .tns appended */
    if (!has_tns) {
        char tns_path[320];
        snprintf(tns_path, sizeof(tns_path), "%s.tns", path);
        r = __real_stat(tns_path, buf);
    }
    return r;
}

/* access() wrapper - check file existence with .tns fallback */
extern int __real_access(const char *path, int amode);

int __wrap_access(const char *path, int amode) {
    size_t len = strlen(path);
    int has_tns = (len >= 4 && strcmp(path + len - 4, ".tns") == 0);

    int r = __real_access(path, amode);
    if (r == 0) return 0;

    if (!has_tns) {
        char tns_path[320];
        snprintf(tns_path, sizeof(tns_path), "%s.tns", path);
        r = __real_access(tns_path, amode);
    }
    return r;
}

/*
 * clock() wrapper - the Nspire newlib clock() accesses timer hardware
 * in a way that causes Data abort (unaligned access) on ARM926EJ-S.
 */
clock_t __wrap_clock(void) {
    return 0;
}
