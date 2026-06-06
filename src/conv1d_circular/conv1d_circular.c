/**
 * conv1d_circular.c — Boolean Circular Convolution
 *
 * Pure boolean kernel: each bit selects whether to include a neighbor.
 * Output[i] = sum of input values at selected neighbor positions (capped at 255).
 * Fully trainable via Tsetlin bit-flip on kernel bits.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "conv1d_circular.h"

/* Get kernel bit k (0 or 1) */
static inline int kbit(const Conv1D *c, uint32_t k) {
    return (c->kernel_bits[k / 8] >> (k % 8)) & 1;
}

/* Flip kernel bit k */
static inline void kflip(Conv1D *c, uint32_t k) {
    c->kernel_bits[k / 8] ^= (uint8_t)(1u << (k % 8));
}

Conv1D* conv1d_create(LayerUID uid, uint32_t N, uint32_t K, uint32_t stride, uint32_t dilation)
{
    if (!N || !K || !stride || !dilation) return NULL;
    Conv1D *c = (Conv1D*)calloc(1, sizeof(Conv1D));
    if (!c) return NULL;
    c->uid = uid; c->input_length = N; c->kernel_size = K;
    c->stride = stride; c->dilation = dilation;
    uint32_t kbytes = (K + 7) / 8;
    c->kernel_bits = (uint8_t*)calloc(kbytes, 1);
    if (!c->kernel_bits) { free(c); return NULL; }
    /* Default: kernel[0]=1 → identity-like (pass first neighbor) */
    c->kernel_bits[0] = 0x01;
    return c;
}

void conv1d_destroy(Conv1D *c) { if(c){ free(c->kernel_bits); free(c); } }

void conv1d_forward(const Conv1D *c, const uint8_t *input, uint8_t *output)
{
    if (!c || !input || !output) return;
    uint32_t out_len = (c->input_length + c->stride - 1) / c->stride;
    for (uint32_t i = 0; i < out_len; i++) {
        uint32_t sum = 0;
        for (uint32_t k = 0; k < c->kernel_size; k++) {
            if (kbit(c, k)) {
                /* Circular index: (i*stride + k*dilation) mod input_length */
                uint32_t idx = (i * c->stride + k * c->dilation) % c->input_length;
                sum += input[idx];
            }
        }
        if (sum > 255) sum = 255;
        output[i] = (uint8_t)sum;
    }
}

int conv1d_forward_layer(void *inst, const uint8_t *in, uint8_t *out)
{
    conv1d_forward((Conv1D*)inst, in, out);
    return 0;
}

/* Tsetlin: flip kernel bit, return 0 on success */
int conv1d_flip_kernel_bit(Conv1D *c, uint32_t bit_idx)
{
    if (!c || bit_idx >= c->kernel_size) return -1;
    kflip(c, bit_idx);
    return 0;
}

int conv1d_save(const Conv1D *c, const char *path)
{
    if (!c || !path) return -1;
    FILE *f = fopen(path, "wb"); if (!f) return -2;
    uint32_t u=c->uid, n=c->input_length, k=c->kernel_size, s=c->stride, d=c->dilation;
    fwrite(&u,4,1,f); fwrite(&n,4,1,f); fwrite(&k,4,1,f);
    fwrite(&s,4,1,f); fwrite(&d,4,1,f);
    fwrite(c->kernel_bits, 1, (k+7)/8, f);
    fclose(f); return 0;
}

Conv1D* conv1d_load(const char *path)
{
    if (!path) return NULL;
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    uint32_t u,n,k,s,d;
    if (fread(&u,4,1,f)!=1||fread(&n,4,1,f)!=1||fread(&k,4,1,f)!=1||
        fread(&s,4,1,f)!=1||fread(&d,4,1,f)!=1) { fclose(f); return NULL; }
    Conv1D *c = conv1d_create(u,n,k,s,d); if (!c) { fclose(f); return NULL; }
    uint32_t kb = (k+7)/8;
    if (fread(c->kernel_bits,1,kb,f)!=kb) { conv1d_destroy(c); fclose(f); return NULL; }
    fclose(f); return c;
}
