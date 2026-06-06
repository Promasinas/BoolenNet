#include <stdlib.h>
#include <string.h>
#include "upsampling.h"

UpsampleLayer* upsample_create(LayerUID uid, uint32_t M, uint32_t N) {
    if (!M || !N) return NULL;
    UpsampleLayer *u = (UpsampleLayer*)calloc(1, sizeof(UpsampleLayer));
    if (!u) return NULL;
    u->uid = uid; u->num_routers = M; u->input_length = N;
    u->routers = (BoolRouter*)calloc(M, sizeof(BoolRouter));
    if (!u->routers) { free(u); return NULL; }
    for (uint32_t i = 0; i < M; i++) {
        u->routers[i].length = N;
        u->routers[i].bits = (uint8_t*)calloc(N, 1);
        if (!u->routers[i].bits) { upsample_destroy(u); return NULL; }
    }
    return u;
}
void upsample_destroy(UpsampleLayer *u) {
    if (!u) return;
    if (u->routers) for (uint32_t i = 0; i < u->num_routers; i++) free(u->routers[i].bits);
    free(u->routers); free(u);
}
int upsample_set_router(UpsampleLayer *u, uint32_t idx, const uint8_t *bits) {
    if (!u || idx >= u->num_routers || !bits) return -1;
    memcpy(u->routers[idx].bits, bits, u->input_length);
    return 0;
}
int upsample_forward(const UpsampleLayer *u, const uint8_t *input, uint8_t *output) {
    if (!u || !input || !output) return -1;
    uint32_t N = u->input_length;
    for (uint32_t m = 0; m < u->num_routers; m++) {
        uint8_t *seg = output + (size_t)m * N;
        const uint8_t *bits = u->routers[m].bits;
        for (uint32_t i = 0; i < N; i++)
            seg[i] = bits[i] ? (uint8_t)(255 - input[i]) : input[i];
    }
    return 0;
}
int upsample_forward_layer(void *inst, const uint8_t *in, uint8_t *out) {
    return upsample_forward((UpsampleLayer*)inst, in, out);
}
int upsample_cascade(UpsampleLayer **layers, uint32_t count, const uint8_t *input, uint8_t *output) {
    if (!layers || !count || !input || !output) return -1;
    uint32_t max_len = layers[0]->input_length;
    for (uint32_t i = 1; i < count; i++)
        max_len *= layers[i]->num_routers;
    uint8_t *b1 = (uint8_t*)malloc(max_len);
    uint8_t *b2 = (uint8_t*)malloc(max_len);
    if (!b1 || !b2) { free(b1); free(b2); return -2; }
    uint8_t *src = b1, *dst = b2;
    uint32_t cur = layers[0]->input_length;
    memcpy(src, input, cur);
    for (uint32_t i = 0; i < count; i++) {
        if (layers[i]->input_length != cur) { free(b1); free(b2); return -3; }
        cur *= layers[i]->num_routers;
        upsample_forward(layers[i], src, dst);
        uint8_t *tmp = src; src = dst; dst = tmp;
    }
    if (src != output) memcpy(output, src, cur);
    free(b1); free(b2); return 0;
}
