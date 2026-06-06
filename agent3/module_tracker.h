#ifndef AGENT3_MODULE_TRACKER_H
#define AGENT3_MODULE_TRACKER_H

#include "agent3.h"

/* Initialize the module tracker. Returns 0 on success. */
int  tracker_init(void);

/* Shutdown and free all resources */
void tracker_shutdown(void);

/* Add or update a test module. timeout_sec from Agent 2 (0 = reject). Returns 0 on success. */
int  tracker_add_module(uint32_t uid, const char *name, const char *src_path, uint32_t timeout_sec);

/* Find module by UID. Returns NULL if not found. */
TestModule *tracker_find_by_uid(uint32_t uid);

/* Update module state */
void tracker_update_state(uint32_t uid, ModuleState state);

/* Check if this UID is a duplicate within dedup_window_sec.
   Returns 1 if duplicate (should be skipped), 0 if new. */
int  tracker_is_duplicate(uint32_t uid, int dedup_window_sec);

/* List pending modules (state == PENDING). Returns count. */
int  tracker_list_pending(TestModule *out, int max_count);

/* Get all tracked modules */
void tracker_get_all(TestModule **out, int *count);

/* Mark a module as tested (update last_tested_at). Force-reset dedup window. */
void tracker_mark_tested(uint32_t uid);

#endif /* AGENT3_MODULE_TRACKER_H */
