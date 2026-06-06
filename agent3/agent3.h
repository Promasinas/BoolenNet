#ifndef AGENT3_H
#define AGENT3_H

#include <stdint.h>

/* Module test states */
typedef enum {
    MODULE_IDLE,
    MODULE_PENDING,
    MODULE_COMPILING,
    MODULE_RUNNING,
    MODULE_DONE,
    MODULE_FAIL
} ModuleState;

/* Pipeline phases */
typedef enum {
    PHASE_DETECT,
    PHASE_CREATE_DIR,
    PHASE_GENERATE_MAKEFILE,
    PHASE_COMPILE,
    PHASE_RUN,
    PHASE_ANALYZE,
    PHASE_REPORT,
    PHASE_COMMIT,
    PHASE_DONE
} PipelinePhase;

/* Test priority from Agent 2 */
typedef enum {
    PRIORITY_LOW,
    PRIORITY_MEDIUM,
    PRIORITY_HIGH
} TestPriority;

/* Test result */
typedef enum {
    RESULT_PASS,
    RESULT_FAIL,
    RESULT_COMPILE_ERROR,
    RESULT_TIMEOUT
} TestResult;

/* A test module tracked by Agent 3 */
typedef struct {
    uint32_t uid;
    char     module_name[64];
    char     src_path[256];
    char     test_dir[256];
    uint64_t last_modified;
    uint64_t last_tested_at;
    uint32_t timeout_sec;   /* from Agent 2 Test Params, 0 = not set (reject) */
    ModuleState state;
} TestModule;

/* Result of compilation + run for a pipeline */
typedef struct {
    int      compile_ok;
    char    *compile_output;     /* heap allocated */
    uint32_t compile_duration_ms;
    int      run_ok;
    char    *run_output;         /* heap allocated */
    uint32_t run_duration_ms;
    int      exit_code;
    int      timed_out;
    uint32_t pass_count;
    uint32_t fail_count;
} PipelineResult;

/* A pipeline tracking one module through all phases */
typedef struct {
    uint32_t       pipeline_id;
    uint32_t       module_uid;
    PipelinePhase  phase;
    uint64_t       start_time;
    void          *process_handle;  /* HANDLE (win32) */
    PipelineResult result;
} Pipeline;

#endif /* AGENT3_H */
