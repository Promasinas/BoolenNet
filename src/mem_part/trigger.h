#ifndef TRIGGER_H
#define TRIGGER_H
#include "part_types.h"
#include "partition.h"
int trigger_compute(const partition_store_t *ps, const trigger_pattern_t *pattern, float *out);
#endif
