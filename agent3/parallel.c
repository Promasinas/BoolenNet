#include "parallel.h"
#include "compiler.h"
#include "runner.h"
#include "module_tracker.h"
#include "utils.h"
#include <stdlib.h>

int parallel_run_pipelines(TestModule *modules, int count, int max_concurrent) {
    if (count == 0) return 0;
    if (max_concurrent < 1) max_concurrent = 1;

    log_info("Pipeline: processing %d modules (max %d concurrent)", count, max_concurrent);

    /* Simple sequential for now — each module goes through all phases.
       Full parallel with WaitForMultipleObjects would add significant complexity
       and is best done as a follow-up optimization. The architecture is designed
       for it (independent per-module directories, no shared state during compile/run). */
    for (int i = 0; i < count; i++) {
        TestModule *m = &modules[i];
        if (m->state != MODULE_PENDING) continue;

        log_info("Pipeline [%d/%d]: uid_%03u (%s)", i + 1, count, m->uid, m->module_name);

        /* Phase 4: Create test directory + Makefile */
        tracker_update_state(m->uid, MODULE_COMPILING);
        /* testdir and makefile_gen are called from main loop — already done before this */

        /* Phase 5: Compile */
        char *compile_out = NULL;
        uint32_t compile_ms = 0;
        int compile_ok = (compile_module(m->uid, &compile_out, &compile_ms) == 0);

        if (!compile_ok) {
            log_error("Pipeline uid_%03u: compile failed", m->uid);
            tracker_update_state(m->uid, MODULE_FAIL);
            free(compile_out);
            continue;
        }

        /* Phase 5: Run */
        tracker_update_state(m->uid, MODULE_RUNNING);
        char *run_out = NULL;
        uint32_t run_ms = 0;
        int exit_code = 0;
        int timed_out = 0;
        uint32_t timeout = m->timeout_sec > 0 ? m->timeout_sec : 600; /* fallback safety */
        run_test(m->uid, timeout, &run_out, &run_ms, &exit_code, &timed_out);

        /* Phase 6: Reporting and notifications are handled in main loop */
        tracker_mark_tested(m->uid);
        tracker_update_state(m->uid, MODULE_DONE);

        free(compile_out);
        free(run_out);

        log_info("Pipeline uid_%03u: done (compile=%ums, run=%ums)", m->uid, compile_ms, run_ms);
    }

    return 0;
}
