/**
 * boolnet.h — Forward Function & Network Topology (Constitution IX)
 *
 * Layer registration by UID, forward propagation in order,
 * network-level save/load.
 */
#ifndef BOOLNET_H
#define BOOLNET_H
#include <stdint.h>
#include "../common/boolnet_common.h"

/* Forward-declare all layer types */
typedef struct Conv1D        Conv1D;
typedef struct UpsampleLayer UpsampleLayer;
typedef struct MemIntLayer   MemIntLayer;

typedef enum { LAYER_ROUTER=0, LAYER_CONV1D=1, LAYER_UPSAMPLE=2, LAYER_MEMORY=3 } LayerType;

typedef struct {
    LayerType type;
    LayerUID  uid;
    void     *instance;   /* Conv1D* / UpsampleLayer* / MemIntLayer* */
    int     (*forward)(void *instance, const float *in, float *out);
    int     (*save_fn)(void *instance, const char *path);
} BoolNetLayer;

typedef struct {
    LayerUID     uid;
    uint32_t     num_layers;
    uint32_t     capacity;
    uint32_t     input_dim;
    uint32_t     output_dim;
    BoolNetLayer *layers;
} BoolNet;

BoolNet* boolnet_create(LayerUID uid, uint32_t input_dim, uint32_t max_layers);
void     boolnet_destroy(BoolNet *net);
int      boolnet_add_layer(BoolNet *net, LayerType type, LayerUID uid, void *instance,
                           int (*forward)(void*,const float*,float*),
                           int (*save_fn)(void*,const char*));
int      boolnet_forward(BoolNet *net, const float *input, float *output);
int      boolnet_save(const BoolNet *net, const char *path);
BoolNet* boolnet_load(const char *path);

#endif
