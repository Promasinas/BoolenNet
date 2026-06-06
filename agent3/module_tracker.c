#include "module_tracker.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

#define MAX_MODULES   256
#define HASH_SIZE     256

typedef struct ModuleEntry {
    TestModule  module;
    struct ModuleEntry *next;  /* chaining for hash collisions */
} ModuleEntry;

static ModuleEntry *g_hash[HASH_SIZE];
static int g_module_count = 0;

static unsigned int hash_uid(uint32_t uid) {
    return uid % HASH_SIZE;
}

int tracker_init(void) {
    memset(g_hash, 0, sizeof(g_hash));
    g_module_count = 0;
    log_info("Module tracker initialized (max %d modules)", MAX_MODULES);
    return 0;
}

void tracker_shutdown(void) {
    for (int i = 0; i < HASH_SIZE; i++) {
        ModuleEntry *entry = g_hash[i];
        while (entry) {
            ModuleEntry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    memset(g_hash, 0, sizeof(g_hash));
    g_module_count = 0;
    log_info("Module tracker shutdown");
}

int tracker_add_module(uint32_t uid, const char *name, const char *src_path, uint32_t timeout_sec) {
    if (g_module_count >= MAX_MODULES) {
        log_error("Module tracker full (max %d)", MAX_MODULES);
        return -1;
    }

    /* Check if already exists */
    TestModule *existing = tracker_find_by_uid(uid);
    if (existing) {
        /* Update existing */
        strncpy(existing->module_name, name, sizeof(existing->module_name) - 1);
        strncpy(existing->src_path, src_path, sizeof(existing->src_path) - 1);
        existing->last_modified = get_timestamp_ms();
        existing->timeout_sec = timeout_sec;
        if (existing->state == MODULE_DONE || existing->state == MODULE_FAIL) {
            existing->state = MODULE_PENDING;  /* re-test */
        }
        log_debug("Module uid_%03u (%s) updated in tracker", uid, name);
        return 0;
    }

    /* Create new entry */
    ModuleEntry *entry = malloc(sizeof(ModuleEntry));
    if (!entry) {
        log_error("Out of memory adding module uid_%03u", uid);
        return -1;
    }
    memset(entry, 0, sizeof(*entry));
    entry->module.uid = uid;
    strncpy(entry->module.module_name, name, sizeof(entry->module.module_name) - 1);
    strncpy(entry->module.src_path, src_path, sizeof(entry->module.src_path) - 1);
    entry->module.last_modified = get_timestamp_ms();
    entry->module.last_tested_at = 0;
    entry->module.timeout_sec = timeout_sec;
    entry->module.state = MODULE_PENDING;

    unsigned int h = hash_uid(uid);
    entry->next = g_hash[h];
    g_hash[h] = entry;
    g_module_count++;

    char uid_dir[32];
    format_uid_dirname(uid, uid_dir, sizeof(uid_dir));
    snprintf(entry->module.test_dir, sizeof(entry->module.test_dir), "./test/%s/", uid_dir);

    log_info("Module uid_%03u (%s) added to tracker, state=PENDING", uid, name);
    return 0;
}

TestModule *tracker_find_by_uid(uint32_t uid) {
    unsigned int h = hash_uid(uid);
    for (ModuleEntry *entry = g_hash[h]; entry; entry = entry->next) {
        if (entry->module.uid == uid) {
            return &entry->module;
        }
    }
    return NULL;
}

void tracker_update_state(uint32_t uid, ModuleState state) {
    TestModule *m = tracker_find_by_uid(uid);
    if (m) {
        ModuleState old = m->state;
        m->state = state;
        log_debug("Module uid_%03u state: %d -> %d", uid, old, state);
    }
}

int tracker_is_duplicate(uint32_t uid, int dedup_window_sec) {
    TestModule *m = tracker_find_by_uid(uid);
    if (!m) return 0;  /* not even tracked, not a duplicate */

    /* If never tested, not a duplicate */
    if (m->last_tested_at == 0) return 0;

    /* If currently in progress, it's a duplicate */
    if (m->state == MODULE_COMPILING || m->state == MODULE_RUNNING) {
        return 1;
    }

    uint64_t now = get_timestamp_ms();
    uint64_t window_ms = (uint64_t)dedup_window_sec * 1000;
    if ((now - m->last_tested_at) < window_ms) {
        log_debug("Module uid_%03u duplicate (last tested %llu ms ago)", uid,
                  (unsigned long long)(now - m->last_tested_at));
        return 1;
    }
    return 0;
}

int tracker_list_pending(TestModule *out, int max_count) {
    int count = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        for (ModuleEntry *entry = g_hash[i]; entry; entry = entry->next) {
            if (entry->module.state == MODULE_PENDING) {
                if (count < max_count) {
                    out[count] = entry->module;
                }
                count++;
            }
        }
    }
    return count;
}

void tracker_get_all(TestModule **out, int *count) {
    static TestModule buf[MAX_MODULES];
    int n = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        for (ModuleEntry *entry = g_hash[i]; entry; entry = entry->next) {
            if (n < MAX_MODULES) {
                buf[n++] = entry->module;
            }
        }
    }
    *out = buf;
    *count = n;
}

void tracker_mark_tested(uint32_t uid) {
    TestModule *m = tracker_find_by_uid(uid);
    if (m) {
        m->last_tested_at = get_timestamp_ms();
    }
}
