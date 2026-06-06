#ifndef AGENT3_REPORTER_H
#define AGENT3_REPORTER_H
#include "agent3.h"
/* Parse test output and write structured report to ./docs/test/uid_NNN_report.md */
int reporter_generate(uint32_t uid, const char *module_name, const char *compile_output, uint32_t compile_ms, int compile_ok, const char *run_output, uint32_t run_ms, int exit_code, int timed_out);
#endif
