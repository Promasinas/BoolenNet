#include "config.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

int get_cpu_core_count(void) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return (n > 0) ? (int)n : 1;
#endif
}

int config_init(Agent3Config *cfg) {
    memset(cfg, 0, sizeof(*cfg));

    cfg->poll_interval_sec = 60;
    cfg->max_concurrent = get_cpu_core_count();
    if (cfg->max_concurrent < 1) cfg->max_concurrent = 1;
    cfg->dedup_window_sec = 60;
    cfg->heartbeat_timeout_sec = 180;
    cfg->verbose = 0;
    cfg->run_once = 0;

    snprintf(cfg->src_dir,        sizeof(cfg->src_dir),        "./src/");
    snprintf(cfg->interact_dir,   sizeof(cfg->interact_dir),   "./docs/interact/");
    snprintf(cfg->test_dir,       sizeof(cfg->test_dir),       "./test/");
    snprintf(cfg->docs_test_dir,  sizeof(cfg->docs_test_dir),  "./docs/test/");
    snprintf(cfg->heartbeat_file, sizeof(cfg->heartbeat_file), "./docs/interact/.agent3_heartbeat");

    return 0;
}

int config_parse_args(Agent3Config *cfg, int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--poll-interval") == 0 && i + 1 < argc) {
            cfg->poll_interval_sec = atoi(argv[++i]);
            if (cfg->poll_interval_sec < 10) cfg->poll_interval_sec = 10;
        } else if (strcmp(argv[i], "--max-concurrent") == 0 && i + 1 < argc) {
            cfg->max_concurrent = atoi(argv[++i]);
            if (cfg->max_concurrent < 1) cfg->max_concurrent = 1;
        } else if (strcmp(argv[i], "--once") == 0) {
            cfg->run_once = 1;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            cfg->verbose = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Agent 3 - BoolNet Test Automation\n");
            printf("Usage: agent3.exe [options]\n");
            printf("  --poll-interval N    Polling interval in seconds (default 60, min 10)\n");
            printf("  --max-concurrent N   Max parallel pipelines (default CPU cores)\n");
            printf("  --once               Run one cycle then exit (debug mode)\n");
            printf("  --verbose            Verbose log output\n");
            printf("  --help, -h           Show this help\n");
            exit(0);
        }
    }
    return 0;
}

void config_print(const Agent3Config *cfg) {
    log_info("=== Agent 3 Configuration ===");
    log_info("  Poll Interval:   %d sec", cfg->poll_interval_sec);
    log_info("  Max Concurrent:  %d", cfg->max_concurrent);
    log_info("  Dedup Window:    %d sec", cfg->dedup_window_sec);
    log_info("  Heartbeat TO:    %d sec", cfg->heartbeat_timeout_sec);
    log_info("  Verbose:         %s", cfg->verbose ? "yes" : "no");
    log_info("  Run Once:        %s", cfg->run_once ? "yes" : "no");
    log_info("  Src Dir:         %s", cfg->src_dir);
    log_info("  Interact Dir:    %s", cfg->interact_dir);
    log_info("  Test Dir:        %s", cfg->test_dir);
    log_info("  Docs Test Dir:   %s", cfg->docs_test_dir);
    log_info("==============================");
}
