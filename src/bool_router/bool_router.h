/**
 * bool_router.h — Boolean Router (Constitution IV)
 *
 * Per-bit XOR routing: output[i] = input[i] ^ router_bits[i].
 * Supports bit-flip for Tsetlin training.
 */
#ifndef BOOL_ROUTER_H
#define BOOL_ROUTER_H
#include <stdint.h>
#include "../common/boolnet_common.h"

typedef struct {
    LayerUID  uid;
    uint32_t  num_bits;    /* bit-length (not byte-length) */
    uint8_t  *bits;        /* ceil(num_bits/8) bytes, LSB-first */
} BoolRouter;

BoolRouter* bool_router_create(LayerUID uid, uint32_t num_bits, const uint8_t *bits);
void        bool_router_destroy(BoolRouter *r);
void        bool_router_forward(const BoolRouter *r, const uint8_t *input, uint8_t *output);

/* Tsetlin support */
int  bool_router_flip_bit(BoolRouter *r, uint32_t bit_idx);
int  bool_router_save(const BoolRouter *r, const char *path);
BoolRouter* bool_router_load(const char *path);

/* BoolNet layer adaptor: uint8_t[N] in → XOR → uint8_t[N] out */
int bool_router_forward_layer(void *inst, const uint8_t *in, uint8_t *out);

#endif
