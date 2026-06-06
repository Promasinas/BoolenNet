#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "trigger.h"
static inline float clamp_w(float w) {
    if (w >  1.0f) return  1.0f;
    if (w < -1.0f) return -1.0f;
    return w;
}
int trigger_compute(const partition_store_t *ps, const trigger_pattern_t *pattern, float *out) {
    if (!ps || !pattern || !out) return PART_ERR_NULL_PARAM;
    if (!ps->initialized) return PART_ERR_NOT_INIT;
    if (pattern->output_dim != ps->activation_dim) return PART_ERR_DIM_MISMATCH;
    uint32_t D = ps->activation_dim;
    memset(out, 0, (size_t)D * sizeof(float));
    if (pattern->num_entries == 0) return PART_OK;
    uint32_t tc = 0;
    uint32_t *ta = (uint32_t*)malloc((size_t)pattern->num_entries * sizeof(uint32_t));
    float    *tw = (float*)   malloc((size_t)pattern->num_entries * sizeof(float));
    if (!ta || !tw) { free(ta); free(tw); return PART_ERR_ALLOC; }
    for (uint32_t i = 0; i < pattern->num_entries; i++) {
        uint32_t a = pattern->entries[i].address;
        if (!partition_addr_valid(ps, a)) continue;
        if (!partition_is_occupied(ps, a)) continue;
        ta[tc] = a; tw[tc] = clamp_w(pattern->entries[i].weight); tc++;
    }
    for (uint32_t t = 0; t < tc; t++) {
        float *vec = partition_get_ptr(ps, ta[t]); float w = tw[t];
        for (uint32_t j = 0; j < D; j++) out[j] += w * vec[j];
    }
    free(ta); free(tw); return PART_OK;
}
