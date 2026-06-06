/**
 * conv1d_circular.h — 1D Circular Convolution (Constitution V)
 *
 * Kernel overflow wraps around to the beginning, guaranteeing each input
 * position is processed exactly K times.
 * Padding is ignored (circular). Stride and dilation are supported.
 */
#ifndef CONV1D_CIRCULAR_H
#define CONV1D_CIRCULAR_H
#include <stdint.h>
#include "../common/boolnet_common.h"

typedef struct {
    LayerUID  uid;
    uint32_t  input_length;    /* N                           */
    uint32_t  kernel_size;     /* K                           */
    uint32_t  stride;          /* output step (default 1)     */
    uint32_t  dilation;        /* kernel spacing (default 1)  */
    float    *kernel;          /* length K                    */
    float    *output;          /* length ceil(N/stride)       */
} Conv1D;

/* Lifecycle */
Conv1D* conv1d_create(LayerUID uid, uint32_t N, uint32_t K, uint32_t stride, uint32_t dilation);
void    conv1d_destroy(Conv1D *c);

/* Forward: output[i] = Σ_{k=0}^{K-1} kernel[k] * input[(i*stride + k*dilation) % N] */
int conv1d_forward(const Conv1D *c, const float *input, float *output);

/* Persistence */
int conv1d_save(const Conv1D *c, const char *path);
Conv1D* conv1d_load(const char *path);

#endif
