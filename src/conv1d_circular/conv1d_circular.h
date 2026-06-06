/**
 * conv1d_circular.h — 1D Boolean Circular Convolution (Constitution V)
 *
 * Pure Boolean: kernel is a bitmask (uint8_t), each bit selects whether
 * to include that neighbor in the sum. Output = count of selected neighbors.
 * Fully trainable via Tsetlin bit-flip on kernel bits.
 */
#ifndef CONV1D_CIRCULAR_H
#define CONV1D_CIRCULAR_H
#include <stdint.h>
#include "../common/boolnet_common.h"

typedef struct {
    LayerUID  uid;
    uint32_t  input_length;   /* N: input byte count */
    uint32_t  kernel_size;    /* K: kernel length (bits) */
    uint32_t  stride;
    uint32_t  dilation;
    uint8_t  *kernel_bits;    /* kernel_size bits packed as bytes, LSB-first. 1=include, 0=exclude */
} Conv1D;

Conv1D* conv1d_create(LayerUID uid, uint32_t N, uint32_t K, uint32_t stride, uint32_t dilation);
void    conv1d_destroy(Conv1D *c);
void    conv1d_forward(const Conv1D *c, const uint8_t *input, uint8_t *output);
int     conv1d_forward_layer(void *inst, const uint8_t *in, uint8_t *out);

/* Tsetlin support: flip one kernel bit */
int     conv1d_flip_kernel_bit(Conv1D *c, uint32_t bit_idx);

int     conv1d_save(const Conv1D *c, const char *path);
Conv1D* conv1d_load(const char *path);

#endif
