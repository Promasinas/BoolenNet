#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "conv1d_circular.h"

Conv1D* conv1d_create(LayerUID uid, uint32_t N, uint32_t K, uint32_t stride, uint32_t dilation) {
    if (!N || !K || !stride || !dilation) return NULL;
    Conv1D *c = (Conv1D*)calloc(1, sizeof(Conv1D));
    if (!c) return NULL;
    c->uid = uid; c->input_length = N; c->kernel_size = K;
    c->stride = stride; c->dilation = dilation;
    c->kernel = (float*)calloc(K, sizeof(float));
    c->output = (float*)calloc((N + stride - 1) / stride, sizeof(float));
    if (!c->kernel || !c->output) { conv1d_destroy(c); return NULL; }
    return c;
}
void conv1d_destroy(Conv1D *c) {
    if (!c) return; free(c->kernel); free(c->output); free(c);
}
int conv1d_forward(const Conv1D *c, const float *input, float *output) {
    if (!c || !input || !output) return -1;
    uint32_t out_len = (c->input_length + c->stride - 1) / c->stride;
    for (uint32_t i = 0; i < out_len; i++) {
        float sum = 0.0f;
        for (uint32_t k = 0; k < c->kernel_size; k++) {
            uint32_t idx = (i * c->stride + k * c->dilation) % c->input_length;
            sum += c->kernel[k] * input[idx];
        }
        output[i] = sum;
    }
    return 0;
}
int conv1d_save(const Conv1D *c, const char *path) {
    if (!c || !path) return -1;
    FILE *f = fopen(path, "wb"); if (!f) return -2;
    uint32_t u=c->uid, n=c->input_length, k=c->kernel_size, s=c->stride, d=c->dilation;
    fwrite(&u,4,1,f); fwrite(&n,4,1,f); fwrite(&k,4,1,f); fwrite(&s,4,1,f); fwrite(&d,4,1,f);
    fwrite(c->kernel, sizeof(float), k, f); fclose(f); return 0;
}
Conv1D* conv1d_load(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    uint32_t u,n,k,s,d;
    if (fread(&u,4,1,f)!=1||fread(&n,4,1,f)!=1||fread(&k,4,1,f)!=1||fread(&s,4,1,f)!=1||fread(&d,4,1,f)!=1) {fclose(f);return NULL;}
    Conv1D *c = conv1d_create(u,n,k,s,d); if (!c) {fclose(f);return NULL;}
    if (fread(c->kernel,sizeof(float),k,f)!=k) {conv1d_destroy(c);fclose(f);return NULL;}
    fclose(f); return c;
}
