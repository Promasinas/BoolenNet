#include "gitmgr.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
static HANDLE g_git_mutex = NULL;
#else
#include <pthread.h>
static pthread_mutex_t g_git_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

int gitmgr_init(void) {
#ifdef _WIN32
    g_git_mutex = CreateMutexA(NULL, FALSE, "Agent3GitMutex");
    if (!g_git_mutex) { log_warn("Cannot create git mutex"); return -1; }
#else
    pthread_mutex_init(&g_git_mutex, NULL);
#endif
    log_info("Git manager initialized");
    return 0;
}

void gitmgr_shutdown(void) {
#ifdef _WIN32
    if (g_git_mutex) CloseHandle(g_git_mutex);
#else
    pthread_mutex_destroy(&g_git_mutex);
#endif
}

static void git_lock(void) {
#ifdef _WIN32
    WaitForSingleObject(g_git_mutex, INFINITE);
#else
    pthread_mutex_lock(&g_git_mutex);
#endif
}

static void git_unlock(void) {
#ifdef _WIN32
    ReleaseMutex(g_git_mutex);
#else
    pthread_mutex_unlock(&g_git_mutex);
#endif
}

int gitmgr_commit(uint32_t uid, const char *module_name) {
    /* Check if there are any changes */
    int has_changes = system("git status --porcelain > nul 2>&1");
    (void)has_changes; /* git status returns 0 if clean, non-zero if dirty or error */

    /* Quick check: try git diff --quiet */
    int status = system("git diff --quiet && git diff --cached --quiet");
    if (status == 0) {
        log_info("Git: no changes to commit for uid_%03u", uid);
        return 0;
    }

    git_lock();

    char uid_dir[32];
    format_uid_dirname(uid, uid_dir, sizeof(uid_dir));
    char time_buf[32];
    get_current_time_str(time_buf, sizeof(time_buf));

    /* git add the test dir and report */
    char add_cmd[512];
    snprintf(add_cmd, sizeof(add_cmd), "git add ./test/%s/ ./docs/test/%s_report.md ./docs/interact/test_done_%s.md 2>&1",
             uid_dir, uid_dir, uid_dir);
    int add_ret = system(add_cmd);

    /* git commit */
    char commit_msg[512];
    snprintf(commit_msg, sizeof(commit_msg),
             "git commit -m \"[Agent3] Test %s (%s) - %s\" 2>&1",
             uid_dir, module_name, time_buf);
    int commit_ret = system(commit_msg);

    git_unlock();

    if (add_ret != 0 || commit_ret != 0) {
        log_error("Git commit failed for uid_%03u (add=%d, commit=%d)", uid, add_ret, commit_ret);
        return -1;
    }

    log_info("Git commit: %s (%s)", uid_dir, module_name);
    return 0;
}
