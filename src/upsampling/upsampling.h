/**
 * upsampling.h — Multi-Router Upsampling (Constitution VI)
 *
 * Splits input across M equal-size Boolean routers, concatenates outputs.
 * Each router produces N bits → total output = M × N bits.
 */
#ifndef UPSAMPLING_H
#define UPSAMPLING_H
#include <stdint.h>
#include "../common/boolnet_common.h"

/* Single Boolean router: array of bits (0=pass, 1=flip) */
typedef struct {
    uint32_t  length;    /* must == input length */
    uint8_t  *bits;      /* length bytes, each 0 or 1 */
} BoolRouter;

typedef struct {
    LayerUID    uid;
    uint32_t    num_routers;   /* M */
    uint32_t    input_length;  /* N */
    BoolRouter *routers;       /* M routers */
    float      *output;        /* M × N floats */
} UpsampleLayer;

UpsampleLayer* upsample_create(LayerUID uid, uint32_t M, uint32_t N);
void           upsample_destroy(UpsampleLayer *u);
int            upsample_set_router(UpsampleLayer *u, uint32_t idx, const uint8_t *bits);
int            upsample_forward(const UpsampleLayer *u, const float *input, float *output);
int            upsample_cascade(UpsampleLayer **layers, uint32_t count, const float *input, float *output);

#endif
