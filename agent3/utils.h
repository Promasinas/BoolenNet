#ifndef AGENT3_UTILS_H
#define AGENT3_UTILS_H

#include <stdint.h>
#include <stdio.h>

/* Log levels */
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

/* Initialize logging with optional file output (NULL = stdout only) */
void log_init(const char *log_file_path);

/* Set minimum log level (default LOG_INFO) */
void log_set_level(LogLevel level);

/* Log a message with timestamp and level */
void log_write(LogLevel level, const char *fmt, ...);

/* Convenience macros */
#define log_debug(...) log_write(LOG_DEBUG, __VA_ARGS__)
#define log_info(...)  log_write(LOG_INFO,  __VA_ARGS__)
#define log_warn(...)  log_write(LOG_WARN,  __VA_ARGS__)
#define log_error(...) log_write(LOG_ERROR, __VA_ARGS__)

/* Get current timestamp in milliseconds */
uint64_t get_timestamp_ms(void);

/* Format a timestamp as ISO 8601 string: "YYYY-MM-DD HH:MM:SS" */
void format_time_iso8601(uint64_t timestamp_ms, char *buf, size_t buf_size);

/* Get current time as formatted string */
void get_current_time_str(char *buf, size_t buf_size);

/* Path utilities */
/* Join path components with appropriate separator */
void path_join(const char *base, const char *rel, char *out, size_t out_size);

/* Convert Windows backslash path to Unix forward slash */
void path_to_unix(char *path);

/* Convert Unix forward slash path to Windows backslash */
void path_to_windows(char *path);

/* Ensure directory exists; create recursively if needed. Returns 0 on success. */
int ensure_dir(const char *path);

/* Check available disk space on the drive containing `path`.
   Returns available megabytes, or -1 on error. */
int64_t check_disk_space(const char *path, int64_t min_free_mb);

/* Simple string utilities */
int str_startswith(const char *str, const char *prefix);
void str_trim(char *str);
char *str_dup(const char *src);

/* Generate a zero-padded UID directory name: e.g. 1 -> "uid_001" */
void format_uid_dirname(uint32_t uid, char *buf, size_t buf_size);

#endif /* AGENT3_UTILS_H */
