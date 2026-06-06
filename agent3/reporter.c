#include "reporter.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int reporter_generate(uint32_t uid, const char *module_name,
                       const char *compile_output, uint32_t compile_ms, int compile_ok,
                       const char *run_output, uint32_t run_ms, int exit_code, int timed_out) {
    char uid_dir[32], path[256], time_buf[32];
    format_uid_dirname(uid, uid_dir, sizeof(uid_dir));
    snprintf(path, sizeof(path), "./docs/test/%s_report.md", uid_dir);
    get_current_time_str(time_buf, sizeof(time_buf));

    /* Parse run output for PASS/FAIL counts */
    int pass = 0, fail = 0;
    if (run_output) {
        const char *p = run_output;
        while ((p = strstr(p, "[PASS]")) != NULL) { pass++; p += 6; }
        p = run_output;
        while ((p = strstr(p, "[FAIL]")) != NULL) { fail++; p += 6; }
    }

    FILE *f = fopen(path, "w");
    if (!f) { log_error("Cannot write report: %s", path); return -1; }

    fprintf(f, "# Test Report: %s\n\n", module_name);
    fprintf(f, "## Module Info\n\n");
    fprintf(f, "- **UID**: %u\n", uid);
    fprintf(f, "- **Module**: %s\n", module_name);
    fprintf(f, "- **Source**: ./src/%s.c\n\n", module_name);
    fprintf(f, "## Test Time\n\n");
    fprintf(f, "- **Start**: %s\n", time_buf);
    fprintf(f, "- **Duration**: %u ms\n\n", compile_ms + run_ms);
    fprintf(f, "## Compile Result\n\n");
    fprintf(f, "- **Status**: %s\n", compile_ok ? "PASS" : "FAIL");
    fprintf(f, "- **Duration**: %u ms\n\n", compile_ms);
    if (compile_output) {
        fprintf(f, "### Compile Output\n\n```text\n%s\n```\n\n", compile_output);
    }

    if (compile_ok) {
        fprintf(f, "## Run Result\n\n");
        fprintf(f, "- **Status**: %s\n", timed_out ? "TIMEOUT" : (exit_code == 0 ? "PASS" : "FAIL"));
        fprintf(f, "- **Duration**: %u ms\n", run_ms);
        fprintf(f, "- **Exit Code**: %d\n\n", exit_code);
        if (run_output) {
            fprintf(f, "### Run Output\n\n```text\n%s\n```\n\n", run_output);
        }
    }

    fprintf(f, "## Summary\n\n");
    fprintf(f, "- **Total Tests**: %d\n", pass + fail);
    fprintf(f, "- **Passed**: %d\n", pass);
    fprintf(f, "- **Failed**: %d\n", fail);
    if (pass + fail > 0) fprintf(f, "- **Pass Rate**: %.1f%%\n", 100.0 * pass / (pass + fail));
    fprintf(f, "\n");

    if (fail > 0 && run_output) {
        fprintf(f, "## Failed Tests\n\n");
        fprintf(f, "| Test Case | Details |\n");
        fprintf(f, "|-----------|--------|\n");
        const char *p = run_output;
        while ((p = strstr(p, "[FAIL]"))) {
            const char *end = strchr(p, '\n');
            if (!end) end = p + strlen(p);
            fprintf(f, "| %.*s |\n", (int)(end - p - 6), p + 6);
            p = end;
        }
        fprintf(f, "\n");
    }

    if (!compile_ok || timed_out) {
        fprintf(f, "## Errors\n\n");
        if (!compile_ok) fprintf(f, "Compilation failed.\n");
        if (timed_out) fprintf(f, "Test execution timed out.\n");
    }

    fclose(f);
    log_info("Report generated: %s (pass=%d, fail=%d)", path, pass, fail);
    return 0;
}
