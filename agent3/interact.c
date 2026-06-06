#include "interact.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

/* Parse a single Markdown test request file */
static TestRequest parse_request_file(const char *filepath) {
    TestRequest req;
    memset(&req, 0, sizeof(req));
    req.valid = 0;

    FILE *f = fopen(filepath, "r");
    if (!f) {
        snprintf(req.error_msg, sizeof(req.error_msg), "Cannot open file: %s", filepath);
        return req;
    }

    char line[512];
    char section[64] = "";
    int has_uid = 0, has_module = 0, has_priority = 0, has_timeout = 0;

    while (fgets(line, sizeof(line), f)) {
        /* Trim trailing newline */
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';

        /* Section header: ## Something */
        if (strncmp(line, "## ", 3) == 0) {
            snprintf(section, sizeof(section), "%s", line + 3);
            continue;
        }

        /* Parse value based on current section */
        if (strcasecmp(section, "Target UID") == 0) {
            if (line[0] && atoi(line) > 0) {
                req.uid = (uint32_t)atoi(line);
                has_uid = 1;
            }
        } else if (strcasecmp(section, "Target Module") == 0) {
            if (line[0]) {
                strncpy(req.module_name, line, sizeof(req.module_name) - 1);
                has_module = 1;
            }
        } else if (strcasecmp(section, "Test Priority") == 0) {
            if (strstr(line, "high") || strstr(line, "High") || strstr(line, "HIGH"))
                req.priority = PRIORITY_HIGH;
            else if (strstr(line, "low") || strstr(line, "Low") || strstr(line, "LOW"))
                req.priority = PRIORITY_LOW;
            else
                req.priority = PRIORITY_MEDIUM;
            has_priority = 1;
        } else if (strcasecmp(section, "Test Params") == 0) {
            /* Parse: "- timeout: 60" or "- cflags: ..." */
            if (strstr(line, "timeout:")) {
                const char *val = strstr(line, "timeout:") + 8;
                while (*val == ' ') val++;
                int t = atoi(val);
                if (t > 0) {
                    req.timeout_sec = (uint32_t)t;
                    has_timeout = 1;
                }
            }
            if (strstr(line, "cflags:")) {
                const char *val = strstr(line, "cflags:") + 7;
                while (*val == ' ') val++;
                strncpy(req.extra_cflags, val, sizeof(req.extra_cflags) - 1);
            }
        }
    }
    fclose(f);

    /* Validate required fields */
    if (!has_uid) {
        snprintf(req.error_msg, sizeof(req.error_msg), "Missing '## Target UID'");
    } else if (!has_module) {
        snprintf(req.error_msg, sizeof(req.error_msg), "Missing '## Target Module'");
    } else if (!has_priority) {
        snprintf(req.error_msg, sizeof(req.error_msg), "Missing '## Test Priority'");
    } else if (!has_timeout) {
        snprintf(req.error_msg, sizeof(req.error_msg), "Missing 'timeout' in '## Test Params'");
    } else {
        req.valid = 1;
    }

    return req;
}

int scan_interact_dir(const char *interact_dir, TestRequest **requests) {
    /* Count matching files first, then parse */
    #define MAX_REQ 128
    static TestRequest req_buf[MAX_REQ];
    int count = 0;

#ifdef _WIN32
    char search[512];
    snprintf(search, sizeof(search), "%stest_request_*.md", interact_dir);
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (count >= MAX_REQ) break;

        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s%s", interact_dir, fd.cFileName);

        TestRequest req = parse_request_file(fullpath);
        if (req.valid) {
            req_buf[count] = req;
            log_info("interact: parsed request uid_%03u (%s), timeout=%us",
                     req.uid, req.module_name, req.timeout_sec);
            count++;
        } else {
            log_warn("interact: invalid request %s: %s", fd.cFileName, req.error_msg);
            /* Still add to list so caller can log/report errors */
            req_buf[count] = req;
            count++;
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
#else
    DIR *d = opendir(interact_dir);
    if (!d) return 0;
    struct dirent *de;
    while ((de = readdir(d)) != NULL && count < MAX_REQ) {
        if (!strstr(de->d_name, "test_request_") || !strstr(de->d_name, ".md")) continue;
        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s%s", interact_dir, de->d_name);
        TestRequest req = parse_request_file(fullpath);
        req_buf[count] = req;
        if (req.valid) {
            log_info("interact: parsed request uid_%03u (%s)", req.uid, req.module_name);
        } else {
            log_warn("interact: invalid request %s: %s", de->d_name, req.error_msg);
        }
        count++;
    }
    closedir(d);
#endif

    *requests = req_buf;
    return count;
}

void free_requests(TestRequest *requests, int count) {
    (void)requests;
    (void)count;
    /* static buffer, no-op */
}

static const char *result_str(TestResult r) {
    switch (r) {
        case RESULT_PASS:          return "pass";
        case RESULT_FAIL:          return "fail";
        case RESULT_COMPILE_ERROR: return "compile_error";
        case RESULT_TIMEOUT:       return "timeout";
        default:                   return "unknown";
    }
}

int write_completion_notification(const char *interact_dir, uint32_t uid,
                                   const char *module_name, TestResult result,
                                   int pass_count, int fail_count,
                                   uint32_t total_duration_ms) {
    char filepath[512];
    char uid_dir[32];
    format_uid_dirname(uid, uid_dir, sizeof(uid_dir));
    snprintf(filepath, sizeof(filepath), "%stest_done_%s.md", interact_dir, uid_dir);

    FILE *f = fopen(filepath, "w");
    if (!f) {
        log_error("Cannot write completion notification: %s", filepath);
        return -1;
    }

    fprintf(f, "# Test Complete: %s\n\n", module_name);
    fprintf(f, "## Status\n\ncompleted\n\n");
    fprintf(f, "## Module\n\n%s (%s)\n\n", uid_dir, module_name);
    fprintf(f, "## Result\n\n%s\n\n", result_str(result));
    fprintf(f, "## Summary\n\n");
    fprintf(f, "- Total: %d tests\n", pass_count + fail_count);
    fprintf(f, "- Passed: %d\n", pass_count);
    fprintf(f, "- Failed: %d\n", fail_count);
    fprintf(f, "- Duration: %u ms\n", total_duration_ms);
    fprintf(f, "- Report: ./docs/test/uid_%03u_report.md\n", uid);
    fclose(f);

    log_info("Completion notification written: %s", filepath);
    return 0;
}

int write_error_report(const char *interact_dir, uint32_t uid,
                        const char *module_name, const char *error_detail) {
    char filepath[512];
    char uid_dir[32];
    format_uid_dirname(uid, uid_dir, sizeof(uid_dir));
    snprintf(filepath, sizeof(filepath), "%serror_%s.md", interact_dir, uid_dir);

    FILE *f = fopen(filepath, "w");
    if (!f) {
        log_error("Cannot write error report: %s", filepath);
        return -1;
    }

    fprintf(f, "# Error Report: %s\n\n", module_name);
    fprintf(f, "## Status\n\nerror\n\n");
    fprintf(f, "## Module\n\n%s (%s)\n\n", uid_dir, module_name);
    fprintf(f, "## Error Detail\n\n%s\n", error_detail);
    fclose(f);

    log_info("Error report written: %s", filepath);
    return 0;
}
