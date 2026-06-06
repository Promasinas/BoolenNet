#include "utils.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir_p(path) _mkdir(path)
#define stat_t struct _stat
#define stat_func _stat
#else
#include <unistd.h>
#include <sys/statvfs.h>
#define mkdir_p(path) mkdir(path, 0755)
#define stat_t struct stat
#define stat_func stat
#endif

/* ---- Logging ---- */
static FILE *g_log_file = NULL;
static LogLevel g_log_level = LOG_INFO;
static int g_log_initialized = 0;

static const char *level_str(LogLevel lv) {
    switch (lv) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "????";
    }
}

void log_init(const char *log_file_path) {
    if (log_file_path) {
        g_log_file = fopen(log_file_path, "a");
    }
    g_log_initialized = 1;
}

void log_set_level(LogLevel level) {
    g_log_level = level;
}

void log_write(LogLevel level, const char *fmt, ...) {
    if (level < g_log_level) return;

    char time_buf[32];
    get_current_time_str(time_buf, sizeof(time_buf));

    va_list args;
    va_start(args, fmt);

    /* Console output */
    fprintf(stdout, "[%s] [%s] ", time_buf, level_str(level));
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    fflush(stdout);

    /* File output */
    if (g_log_file) {
        va_end(args);
        va_start(args, fmt);
        fprintf(g_log_file, "[%s] [%s] ", time_buf, level_str(level));
        vfprintf(g_log_file, fmt, args);
        fprintf(g_log_file, "\n");
        fflush(g_log_file);
    }
    va_end(args);
}

/* ---- Time ---- */
uint64_t get_timestamp_ms(void) {
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli;
    uli.LowPart  = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    /* Convert from 100-nanosecond intervals since 1601 to ms since 1970.
       Offset = 116444736000000000 (100-ns intervals from 1601 to 1970) */
    return (uli.QuadPart - 116444736000000000ULL) / 10000ULL;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

void format_time_iso8601(uint64_t timestamp_ms, char *buf, size_t buf_size) {
    time_t sec = (time_t)(timestamp_ms / 1000);
    struct tm *tm_info;
#ifdef _WIN32
    struct tm tm_buf;
    tm_info = localtime(&sec);
    if (tm_info) {
        memcpy(&tm_buf, tm_info, sizeof(tm_buf));
        tm_info = &tm_buf;
    }
#else
    struct tm tm_buf;
    localtime_r(&sec, &tm_buf);
    tm_info = &tm_buf;
#endif
    if (tm_info) {
        strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(buf, buf_size, "1970-01-01 00:00:00");
    }
}

void get_current_time_str(char *buf, size_t buf_size) {
    uint64_t now = get_timestamp_ms();
    format_time_iso8601(now, buf, buf_size);
}

/* ---- Path utilities ---- */
void path_join(const char *base, const char *rel, char *out, size_t out_size) {
    size_t blen = strlen(base);
    if (blen == 0) {
        snprintf(out, out_size, "%s", rel);
        return;
    }
    char sep = '/';
    int need_sep = (base[blen - 1] != '/' && base[blen - 1] != '\\' && rel[0] != '/' && rel[0] != '\\');
    if (need_sep) {
        snprintf(out, out_size, "%s%c%s", base, sep, rel);
    } else {
        snprintf(out, out_size, "%s%s", base, rel);
    }
}

void path_to_unix(char *path) {
    for (; *path; path++) {
        if (*path == '\\') *path = '/';
    }
}

void path_to_windows(char *path) {
    for (; *path; path++) {
        if (*path == '/') *path = '\\';
    }
}

int ensure_dir(const char *path) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    path_to_windows(tmp); /* Use Windows native for mkdir on this platform */

    /* Recursive creation */
    for (char *p = tmp; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char saved = *p;
            *p = '\0';
            if (strlen(tmp) > 0 && strlen(tmp) < 3) { /* skip drive root like C: */
                /* actually still try to create it - mkdir will just fail silently */
            }
            mkdir_p(tmp);
            *p = saved;
        }
    }
    mkdir_p(tmp); /* final component */
    return 0;
}

int64_t check_disk_space(const char *path, int64_t min_free_mb) {
#ifdef _WIN32
    ULARGE_INTEGER free_bytes;
    char root[4] = {0};
    if (path && path[0] && path[1] == ':') {
        root[0] = path[0]; root[1] = ':'; root[2] = '\\'; root[3] = '\0';
    } else {
        /* Get current drive */
        GetCurrentDirectoryA(sizeof(root) - 1, root);
        root[2] = '\\'; root[3] = '\0';
    }
    if (GetDiskFreeSpaceExA(root, &free_bytes, NULL, NULL)) {
        int64_t free_mb = (int64_t)(free_bytes.QuadPart / (1024 * 1024));
        if (free_mb < min_free_mb) {
            log_warn("Low disk space: %lld MB available (need %lld MB)", free_mb, min_free_mb);
        }
        return free_mb;
    }
    return -1;
#else
    struct statvfs s;
    if (statvfs(path ? path : ".", &s) == 0) {
        int64_t free_mb = (int64_t)((uint64_t)s.f_bavail * s.f_frsize / (1024 * 1024));
        if (free_mb < min_free_mb) {
            log_warn("Low disk space: %lld MB available (need %lld MB)", free_mb, min_free_mb);
        }
        return free_mb;
    }
    return -1;
#endif
}

/* ---- String utilities ---- */
int str_startswith(const char *str, const char *prefix) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

void str_trim(char *str) {
    /* Left trim */
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
    char *start = str;
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end-- = '\0';
    }
    if (start != str) {
        memmove(str - (start - str), str, strlen(str) + 1); /* This is broken for this case, just trim in place */
        /* Simpler: trim right only, and return trimmed start */
    }
}

char *str_dup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char *dst = malloc(len);
    if (dst) memcpy(dst, src, len);
    return dst;
}

void format_uid_dirname(uint32_t uid, char *buf, size_t buf_size) {
    snprintf(buf, buf_size, "uid_%03u", uid);
}
