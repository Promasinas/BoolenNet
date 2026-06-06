#ifndef AGENT3_INTERACT_H
#define AGENT3_INTERACT_H

#include "agent3.h"

/* Parsed test request from Agent 2 */
typedef struct {
    uint32_t uid;
    char     module_name[64];
    TestPriority priority;
    uint32_t timeout_sec;
    char     extra_cflags[256];  /* optional */
    int      valid;              /* 1 = all required fields present */
    char     error_msg[256];     /* parse error description if !valid */
} TestRequest;

/* Scan ./docs/interact/ for test_request_uid_*.md files.
   Returns list of parsed requests. Caller must free with free_requests(). */
int scan_interact_dir(const char *interact_dir, TestRequest **requests);

/* Free request list returned by scan_interact_dir */
void free_requests(TestRequest *requests, int count);

/* Generate completion notification: ./docs/interact/test_done_uid_NNN.md */
int write_completion_notification(const char *interact_dir, uint32_t uid,
                                   const char *module_name, TestResult result,
                                   int pass_count, int fail_count,
                                   uint32_t total_duration_ms);

/* Generate error report: ./docs/interact/error_uid_NNN.md */
int write_error_report(const char *interact_dir, uint32_t uid,
                        const char *module_name, const char *error_detail);

#endif /* AGENT3_INTERACT_H */
