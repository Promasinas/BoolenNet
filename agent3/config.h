#ifndef AGENT3_CONFIG_H
#define AGENT3_CONFIG_H

#include <stdint.h>

/* Agent 3 runtime configuration */
typedef struct {
    int poll_interval_sec;    /* polling cycle interval (default 60) */
    int max_concurrent;       /* max parallel pipelines (default = CPU cores) */
    int dedup_window_sec;     /* dedup window (default 60) */
    int heartbeat_timeout_sec;/* heartbeat stale threshold (default 180) */
    int verbose;              /* verbose logging flag */
    int run_once;             /* single cycle then exit (debug mode) */

    /* Directory paths */
    char src_dir[256];
    char interact_dir[256];
    char test_dir[256];
    char docs_test_dir[256];
    char heartbeat_file[256];
} Agent3Config;

/* Initialize config with defaults. Returns 0 on success. */
int config_init(Agent3Config *cfg);

/* Parse command-line arguments into config. Returns 0 on success. */
int config_parse_args(Agent3Config *cfg, int argc, char *argv[]);

/* Print current configuration summary to log */
void config_print(const Agent3Config *cfg);

/* Get CPU core count (portable: Windows GetSystemInfo / sysconf) */
int get_cpu_core_count(void);

#endif /* AGENT3_CONFIG_H */
