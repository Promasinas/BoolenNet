#include "detector.h"
#include "module_tracker.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

/* Extract module name from filename: "bool_router.c" -> "bool_router" */
static void extract_module_name(const char *filename, char *out, size_t out_size) {
    const char *dot = strrchr(filename, '.');
    size_t len = dot ? (size_t)(dot - filename) : strlen(filename);
    if (len >= out_size) len = out_size - 1;
    memcpy(out, filename, len);
    out[len] = '\0';
}

/* Check if file has a source extension we care about */
static int is_source_file(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;
    return (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0);
}

int scan_src_dir(const char *src_dir, TestModule *modules, int max_count) {
    int count = 0;

#ifdef _WIN32
    char search_path[512];
    snprintf(search_path, sizeof(search_path), "%s*", src_dir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        log_debug("scan_src_dir: no files found in %s (empty or missing)", src_dir);
        return 0;
    }

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (!is_source_file(fd.cFileName)) continue;

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s%s", src_dir, fd.cFileName);

        /* Get modification timestamp */
        ULARGE_INTEGER uli;
        uli.LowPart  = fd.ftLastWriteTime.dwLowDateTime;
        uli.HighPart = fd.ftLastWriteTime.dwHighDateTime;
        uint64_t file_time = (uli.QuadPart - 116444736000000000ULL) / 10000ULL;

        /* Check if already tracked and unchanged */
        char name_buf[64];
        extract_module_name(fd.cFileName, name_buf, sizeof(name_buf));
        /* We need a UID to look up — use a simple heuristic: hash filename */
        /* In production, UID comes from interact docs. For file-based detection,
           we assign a temporary UID from the filename. Agent 2 will provide
           the real UID via test_request documents. */
        uint32_t uid = 0;
        /* Try to find an existing tracked module by source path */
        TestModule *all;
        int all_count;
        tracker_get_all(&all, &all_count);
        for (int i = 0; i < all_count; i++) {
            if (strcmp(all[i].src_path, full_path) == 0) {
                uid = all[i].uid;
                break;
            }
        }

        if (uid != 0) {
            /* Already tracked — check if modified since last test */
            if (tracker_is_duplicate(uid, 60)) {
                log_debug("scan_src_dir: skipping duplicate uid_%03u (%s)", uid, fd.cFileName);
                continue;
            }
            TestModule *m = tracker_find_by_uid(uid);
            if (m && file_time <= m->last_modified) {
                log_debug("scan_src_dir: %s unchanged, skipping", fd.cFileName);
                continue;
            }
        }

        /* New or changed module */
        if (count < max_count) {
            modules[count].uid = uid;  /* 0 if new, will be assigned from interact doc */
            extract_module_name(fd.cFileName, modules[count].module_name, sizeof(modules[count].module_name));
            snprintf(modules[count].src_path, sizeof(modules[count].src_path), "%s%s", src_dir, fd.cFileName);
            modules[count].last_modified = file_time;
            modules[count].state = MODULE_IDLE;
            count++;
        }
        log_info("scan_src_dir: detected %s (time=%llu)", fd.cFileName, (unsigned long long)file_time);
    } while (FindNextFileA(hFind, &fd) && count < max_count);

    FindClose(hFind);
#else
    DIR *d = opendir(src_dir);
    if (!d) {
        log_debug("scan_src_dir: cannot open %s", src_dir);
        return 0;
    }
    struct dirent *de;
    while ((de = readdir(d)) != NULL && count < max_count) {
        if (!is_source_file(de->d_name)) continue;

        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s%s", src_dir, de->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;
        uint64_t file_time = (uint64_t)st.st_mtime * 1000;

        /* similar dedup logic */
        uint32_t uid = 0;
        TestModule *all; int all_count;
        tracker_get_all(&all, &all_count);
        for (int i = 0; i < all_count; i++) {
            if (strcmp(all[i].src_path, full_path) == 0) { uid = all[i].uid; break; }
        }
        if (uid != 0 && tracker_is_duplicate(uid, 60)) continue;

        if (count < max_count) {
            modules[count].uid = uid;
            extract_module_name(de->d_name, modules[count].module_name, sizeof(modules[count].module_name));
            snprintf(modules[count].src_path, sizeof(modules[count].src_path), "%s%s", src_dir, de->d_name);
            modules[count].last_modified = file_time;
            modules[count].state = MODULE_IDLE;
            count++;
        }
        log_info("scan_src_dir: detected %s", de->d_name);
    }
    closedir(d);
#endif

    return count;
}
