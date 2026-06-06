#ifndef AGENT3_RUNNER_H
#define AGENT3_RUNNER_H
#include <stdint.h>
/* Run test exe. Returns 0 if run completed (success or test failure), -1 on timeout. */
int run_test(uint32_t uid, uint32_t timeout_sec, char **output, uint32_t *duration_ms, int *exit_code, int *timed_out);
#endif
