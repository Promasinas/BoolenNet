#include "heartbeat.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

static char g_heartbeat_path[256];
static int  g_heartbeat_initialized = 0;

int heartbeat_init(const char *filepath) {
    strncpy(g_heartbeat_path, filepath, sizeof(g_heartbeat_path) - 1);
    g_heartbeat_initialized = 1;
    log_info("Heartbeat initialized: %s", g_heartbeat_path);
    return 0;
}

void heartbeat_tick(void) {
    if (!g_heartbeat_initialized) return;

    char time_buf[32];
    get_current_time_str(time_buf, sizeof(time_buf));

    FILE *f = fopen(g_heartbeat_path, "w");
    if (f) {
        fprintf(f, "%s\n", time_buf);
        fclose(f);
        log_debug("Heartbeat written: %s", time_buf);
    } else {
        log_warn("Failed to write heartbeat to %s", g_heartbeat_path);
    }
}

void heartbeat_shutdown(void) {
    /* Remove heartbeat file on clean shutdown */
    if (g_heartbeat_initialized) {
        remove(g_heartbeat_path);
        log_info("Heartbeat file removed (clean shutdown)");
    }
}
