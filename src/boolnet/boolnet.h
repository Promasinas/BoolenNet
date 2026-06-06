/**
 * boolnet.h — Forward Function & Network Topology (Constitution IX)
 *
 * Pure Boolean network: all data flows as uint8_t byte arrays.
 * Layer registration by UID, forward propagation in order.
 */
#ifndef BOOLNET_H
#define BOOLNET_H
#include <stdint.h>
#include "../common/boolnet_common.h"

typedef enum { LAYER_ROUTER=0, LAYER_CONV1D=1, LAYER_UPSAMPLE=2, LAYER_MEMORY=3 } LayerType;

typedef struct {
    LayerType type;
    LayerUID  uid;
    void     *instance;
    int     (*forward)(void *instance, const uint8_t *in, uint8_t *out);
    int     (*save_fn)(void *instance, const char *path);
} BoolNetLayer;

typedef struct {
    LayerUID     uid;
    uint32_t     num_layers, capacity, input_dim, output_dim;
    BoolNetLayer *layers;
} BoolNet;

BoolNet* boolnet_create(LayerUID uid, uint32_t dim, uint32_t max_layers);
void     boolnet_destroy(BoolNet *net);
int      boolnet_add_layer(BoolNet *net, LayerType type, LayerUID uid, void *inst,
                           int (*forward)(void*,const uint8_t*,uint8_t*),
                           int (*save_fn)(void*,const char*));
int      boolnet_forward(BoolNet *net, const uint8_t *input, uint8_t *output);
int      boolnet_save(const BoolNet *net, const char *path);
BoolNet* boolnet_load(const char *path);

#endif
