#include "testdir.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

int testdir_create(uint32_t uid, const char *module_name) {
    char uid_dir[32], path[256];
    format_uid_dirname(uid, uid_dir, sizeof(uid_dir));
    snprintf(path, sizeof(path), "./test/%s/", uid_dir);
    ensure_dir(path);

    /* Write module_info.txt */
    char info_path[512];
    snprintf(info_path, sizeof(info_path), "%smodule_info.txt", path);
    FILE *f = fopen(info_path, "w");
    if (f) {
        fprintf(f, "UID=%u\nModule=%s\n", uid, module_name);
        fclose(f);
    }
    log_info("Test dir created: %s", path);
    return 0;
}

int testdir_generate_framework(uint32_t uid, const char *module_name) {
    char uid_dir[32], path[256];
    format_uid_dirname(uid, uid_dir, sizeof(uid_dir));
    snprintf(path, sizeof(path), "./test/%s/test_main.c", uid_dir);

    FILE *f = fopen(path, "w");
    if (!f) { log_error("Cannot write %s", path); return -1; }

    fprintf(f, "/* Auto-generated test framework for uid_%03u: %s */\n", uid, module_name);
    fprintf(f, "/* Test assertions to be filled by Agent 2 */\n\n");
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n\n");
    fprintf(f, "/* TODO: #include \"../../src/%s.h\" */\n\n", module_name);
    fprintf(f, "int main(void) {\n");
    fprintf(f, "    int passed = 0, failed = 0;\n\n");
    fprintf(f, "    /* Example test pattern:\n");
    fprintf(f, "       printf(\"[TEST] Running test_name...\\n\");\n");
    fprintf(f, "       if (expected == actual) {\n");
    fprintf(f, "           printf(\"[PASS] test_name\\n\");\n");
    fprintf(f, "           passed++;\n");
    fprintf(f, "       } else {\n");
    fprintf(f, "           printf(\"[FAIL] test_name | Input: ... | Expected: ... | Actual: ...\\n\");\n");
    fprintf(f, "           failed++;\n");
    fprintf(f, "       }\n");
    fprintf(f, "    */\n\n");
    fprintf(f, "    printf(\"=== TEST SUMMARY ===\\n\");\n");
    fprintf(f, "    printf(\"Total: %%d | Passed: %%d | Failed: %%d\\n\", passed + failed, passed, failed);\n");
    fprintf(f, "    return (failed > 0) ? 1 : 0;\n");
    fprintf(f, "}\n");
    fclose(f);
    log_info("Test framework generated: %s", path);
    return 0;
}
