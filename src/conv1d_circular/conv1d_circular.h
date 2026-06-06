/**
 * conv1d_circular.h — 1D Circular Convolution (Constitution V)
 *
 * Byte-stream interface (uint8_t) for BoolNet framework.
 * Kernel weights are float; I/O is uint8_t with internal float conversion.
 */
#ifndef CONV1D_CIRCULAR_H
#define CONV1D_CIRCULAR_H
#include <stdint.h>
#include "../common/boolnet_common.h"

typedef struct {
    LayerUID  uid;
    uint32_t  input_length, kernel_size, stride, dilation;
    float    *kernel;      /* length K, learned params */
} Conv1D;

Conv1D* conv1d_create(LayerUID uid, uint32_t N, uint32_t K, uint32_t stride, uint32_t dilation);
void    conv1d_destroy(Conv1D *c);
int     conv1d_forward(const Conv1D *c, const uint8_t *input, uint8_t *output);
int     conv1d_forward_layer(void *inst, const uint8_t *in, uint8_t *out); /* BoolNet adaptor */
int     conv1d_save(const Conv1D *c, const char *path);
Conv1D* conv1d_load(const char *path);

#endif
