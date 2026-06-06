/**
 * upsampling.h — Multi-Router Upsampling (Constitution VI)
 *
 * Byte-stream: M Boolean routers process uint8_t input, outputs concatenated.
 * Each bit=0 → pass (identity), bit=1 → flip (255 - byte).
 */
#ifndef UPSAMPLING_H
#define UPSAMPLING_H
#include <stdint.h>
#include "../common/boolnet_common.h"

typedef struct { uint32_t length; uint8_t *bits; } UpsampleRouter;

typedef struct {
    LayerUID       uid;
    uint32_t       num_routers, input_length;
    UpsampleRouter *routers;
} UpsampleLayer;

UpsampleLayer* upsample_create(LayerUID uid, uint32_t M, uint32_t N);
void           upsample_destroy(UpsampleLayer *u);
int            upsample_set_router(UpsampleLayer *u, uint32_t idx, const uint8_t *bits);
int            upsample_forward(const UpsampleLayer *u, const uint8_t *input, uint8_t *output);
int            upsample_forward_layer(void *inst, const uint8_t *in, uint8_t *out);
int            upsample_cascade(UpsampleLayer **layers, uint32_t count, const uint8_t *input, uint8_t *output);
int            upsample_save(const UpsampleLayer *u, const char *path);
UpsampleLayer* upsample_load(const char *path);

#endif
