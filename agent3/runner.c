#include "runner.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

int run_test(uint32_t uid, uint32_t timeout_sec, char **output, uint32_t *duration_ms, int *exit_code, int *timed_out) {
    char uid_dir[32], exe_name[64], cmd[512], tmpfile[256];
    format_uid_dirname(uid, uid_dir, sizeof(uid_dir));
    snprintf(exe_name, sizeof(exe_name), "test_%s.exe", uid_dir);
    snprintf(tmpfile, sizeof(tmpfile), "./test/%s/run_output_%u.txt", uid_dir, (unsigned)GetCurrentProcessId());
    snprintf(cmd, sizeof(cmd), "\"./test/%s/%s\" > \"%s\" 2>&1", uid_dir, exe_name, tmpfile);

    uint64_t t0 = get_timestamp_ms();

#ifdef _WIN32
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    char cmdline[512];
    snprintf(cmdline, sizeof(cmdline), "cmd /c \"%s\"", cmd);

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        *output = str_dup("(failed to create process)");
        *duration_ms = 0;
        *exit_code = -1;
        *timed_out = 0;
        return -1;
    }

    DWORD timeout_ms = timeout_sec * 1000;
    DWORD wait_result = WaitForSingleObject(pi.hProcess, timeout_ms);

    uint64_t t1 = get_timestamp_ms();
    *duration_ms = (uint32_t)(t1 - t0);

    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        *timed_out = 1;
        *exit_code = -1;
        log_warn("Test TIMEOUT: %s (%us)", uid_dir, timeout_sec);
    } else {
        *timed_out = 0;
        GetExitCodeProcess(pi.hProcess, (LPDWORD)exit_code);
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    /* Unix: use system() with alarm/timeout */
    snprintf(cmd, sizeof(cmd), "timeout %u ./test/%s/%s > \"%s\" 2>&1",
             timeout_sec, uid_dir, exe_name, tmpfile);
    *exit_code = system(cmd);
    uint64_t t1 = get_timestamp_ms();
    *duration_ms = (uint32_t)(t1 - t0);
    *timed_out = (*exit_code == 124) ? 1 : 0; /* timeout returns 124 */
#endif

    /* Read output */
    FILE *f = fopen(tmpfile, "rb");
    if (!f) { *output = str_dup("(no output)"); return *timed_out ? -1 : 0; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    *output = malloc(sz + 1);
    if (*output) { size_t r = fread(*output, 1, sz, f); (*output)[r] = '\0'; }
    fclose(f);
    remove(tmpfile);

    log_info("Test run complete: %s (exit=%d, timeout=%d, %ums)", uid_dir, *exit_code, *timed_out, *duration_ms);
    return *timed_out ? -1 : 0;
}
