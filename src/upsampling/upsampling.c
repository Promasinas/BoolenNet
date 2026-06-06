#include <stdlib.h>
#include <string.h>
#include "upsampling.h"

UpsampleLayer* upsample_create(LayerUID uid, uint32_t M, uint32_t N) {
    if (!M || !N) return NULL;
    UpsampleLayer *u = (UpsampleLayer*)calloc(1, sizeof(UpsampleLayer));
    if (!u) return NULL;
    u->uid = uid; u->num_routers = M; u->input_length = N;
    u->routers = (BoolRouter*)calloc(M, sizeof(BoolRouter));
    u->output  = (float*)calloc((size_t)M * N, sizeof(float));
    if (!u->routers || !u->output) { upsample_destroy(u); return NULL; }
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
    free(u->routers); free(u->output); free(u);
}
int upsample_set_router(UpsampleLayer *u, uint32_t idx, const uint8_t *bits) {
    if (!u || idx >= u->num_routers || !bits) return -1;
    memcpy(u->routers[idx].bits, bits, u->input_length);
    return 0;
}
int upsample_forward(const UpsampleLayer *u, const float *input, float *output) {
    if (!u || !input || !output) return -1;
    uint32_t N = u->input_length;
    for (uint32_t m = 0; m < u->num_routers; m++) {
        float *seg = output + (size_t)m * N;
        const uint8_t *bits = u->routers[m].bits;
        for (uint32_t i = 0; i < N; i++)
            seg[i] = bits[i] ? -input[i] : input[i];  /* flip=negate, pass=identity */
    }
    return 0;
}
int upsample_cascade(UpsampleLayer **layers, uint32_t count, const float *input, float *output) {
    if (!layers || !count || !input || !output) return -1;
    /* allocate temporary buffers for intermediate results */
    uint32_t max_len = layers[0]->input_length;
    for (uint32_t i = 1; i < count; i++) {
        uint32_t len = layers[i]->num_routers * layers[i-1]->num_routers * layers[0]->input_length;
        if (len > max_len) max_len = len;
    }
    float *buf1 = (float*)malloc(max_len * sizeof(float));
    float *buf2 = (float*)malloc(max_len * sizeof(float));
    if (!buf1 || !buf2) { free(buf1); free(buf2); return -2; }
    float *src = buf1, *dst = buf2;
    uint32_t cur_len = layers[0]->input_length;
    memcpy(src, input, cur_len * sizeof(float));
    for (uint32_t i = 0; i < count; i++) {
        if (layers[i]->input_length != cur_len) { free(buf1); free(buf2); return -3; }
        cur_len = layers[i]->num_routers * cur_len;
        upsample_forward(layers[i], src, dst);
        float *tmp = src; src = dst; dst = tmp;
    }
    if (src != output) memcpy(output, src, cur_len * sizeof(float));
    free(buf1); free(buf2);
    return 0;
}
