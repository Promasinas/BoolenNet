#include "compiler.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

static char *run_make_and_capture(const char *work_dir, uint32_t timeout_ms, int *exit_code, uint32_t *duration_ms) {
    uint64_t t0 = get_timestamp_ms();
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "make -C \"%s\" all 2>&1", work_dir);

    char tmpfile[256];
    snprintf(tmpfile, sizeof(tmpfile), "%s/compile_output_%u.txt", work_dir, (unsigned)GetCurrentProcessId());
    /* Append stderr redirection: make ... 2>&1 > file */
    snprintf(cmd, sizeof(cmd), "make -C \"%s\" all > \"%s\" 2>&1", work_dir, tmpfile);

    *exit_code = system(cmd);
    uint64_t t1 = get_timestamp_ms();
    *duration_ms = (uint32_t)(t1 - t0);

    /* Read captured output */
    FILE *f = fopen(tmpfile, "rb");
    if (!f) return str_dup("(no output)");

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *out = malloc(sz + 1);
    if (out) {
        size_t read = fread(out, 1, sz, f);
        out[read] = '\0';
    }
    fclose(f);
    remove(tmpfile);
    return out ? out : str_dup("(read error)");
}

int compile_module(uint32_t uid, char **output, uint32_t *duration_ms) {
    char uid_dir[32], work_dir[256];
    format_uid_dirname(uid, uid_dir, sizeof(uid_dir));
    snprintf(work_dir, sizeof(work_dir), "./test/%s/", uid_dir);

    int exit_code = 0;
    *output = run_make_and_capture(work_dir, 300000, &exit_code, duration_ms);
    if (exit_code == 0) {
        log_info("Compile ok: %s (exit=%d, %ums)", uid_dir, exit_code, *duration_ms);
        return 0;
    } else {
        log_warn("Compile FAIL: %s (exit=%d, %ums)", uid_dir, exit_code, *duration_ms);
        return -1;
    }
}
