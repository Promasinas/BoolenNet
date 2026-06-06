#ifndef AGENT3_PARALLEL_H
#define AGENT3_PARALLEL_H
#include "agent3.h"
/* Run full pipeline for all PENDING modules with max_concurrent limit */
int parallel_run_pipelines(TestModule *modules, int count, int max_concurrent);
#endif
