/**
 * mem_int_layer.h — Integer Memory Layer (Constitution VIII)
 *
 * 1D fixed-length integer memory with uint8/16/32/64 precision.
 * Boolean-router-driven trigger recovery + per-forward decay.
 * Binary persistence (save/load).
 */
#ifndef MEM_INT_LAYER_H
#define MEM_INT_LAYER_H
#include "../common/boolnet_common.h"

typedef struct {
    LayerUID  uid;
    uint8_t   precision;
    uint32_t  length;
    uint64_t  max_value;
    uint64_t  decay;
    void     *cells;
} MemIntLayer;

MemIntLayer* mem_int_create(LayerUID uid, uint8_t precision, uint32_t length, uint64_t max_value, uint64_t decay);
void         mem_int_destroy(MemIntLayer *layer);
void         mem_int_forward(MemIntLayer *layer, const uint8_t *router_signal);
void         mem_int_query(const MemIntLayer *layer, uint8_t *trigger_mask);
int          mem_int_save(const MemIntLayer *layer, const char *filepath);
MemIntLayer* mem_int_load(const char *filepath);
void         mem_int_reset(MemIntLayer *layer);
int          mem_int_all_zero(const MemIntLayer *layer);
int          mem_int_verify_roundtrip(const MemIntLayer *layer);

/* BoolNet layer adaptor: uint8_t[N] input → signal → forward → query → uint8_t[N] output */
int mem_int_forward_layer(void *inst, const uint8_t *in, uint8_t *out);

/* Output layer adaptor: reads raw cell values (not binary mask) */
int mem_int_forward_output(void *inst, const uint8_t *in, uint8_t *out);

#endif
