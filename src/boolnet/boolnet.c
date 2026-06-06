#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "boolnet.h"

BoolNet* boolnet_create(LayerUID uid, uint32_t dim, uint32_t max_layers)
{
    if (!dim || !max_layers) return NULL;
    BoolNet *n = (BoolNet*)calloc(1, sizeof(BoolNet));
    if (!n) return NULL;
    n->uid = uid; n->input_dim = dim; n->output_dim = dim;
    n->capacity = max_layers; n->num_layers = 0;
    n->layers = (BoolNetLayer*)calloc(max_layers, sizeof(BoolNetLayer));
    if (!n->layers) { free(n); return NULL; }
    return n;
}

void boolnet_destroy(BoolNet *n)
{
    if (!n) return;
    free(n->layers);
    free(n);
}

int boolnet_add_layer(BoolNet *n, LayerType type, LayerUID uid, void *inst,
                      int (*forward)(void*,const uint8_t*,uint8_t*),
                      int (*save_fn)(void*,const char*))
{
    if (!n || n->num_layers >= n->capacity) return -1;
    /* Insert sorted by UID */
    uint32_t pos = n->num_layers;
    while (pos > 0 && n->layers[pos-1].uid > uid) {
        n->layers[pos] = n->layers[pos-1];
        pos--;
    }
    n->layers[pos].type = type;
    n->layers[pos].uid = uid;
    n->layers[pos].instance = inst;
    n->layers[pos].forward = forward;
    n->layers[pos].save_fn = save_fn;
    n->num_layers++;
    return 0;
}

int boolnet_forward(BoolNet *n, const uint8_t *input, uint8_t *output)
{
    if (!n || !input || !output) return -1;
    if (n->num_layers == 0) {
        memcpy(output, input, n->input_dim);
        return 0;
    }

    /* Double-buffer for layer-to-layer propagation */
    uint8_t *buf[2];
    buf[0] = (uint8_t*)malloc(n->input_dim * 4);
    buf[1] = (uint8_t*)malloc(n->input_dim * 4);
    if (!buf[0] || !buf[1]) { free(buf[0]); free(buf[1]); return -2; }

    memcpy(buf[0], input, n->input_dim);
    int src = 0;
    for (uint32_t i = 0; i < n->num_layers; i++) {
        int rc = n->layers[i].forward(n->layers[i].instance, buf[src], buf[1-src]);
        if (rc) { free(buf[0]); free(buf[1]); return rc; }
        src = 1 - src;
    }
    memcpy(output, buf[src], n->output_dim);
    free(buf[0]); free(buf[1]);
    return 0;
}

int boolnet_save(const BoolNet *n, const char *path)
{
    if (!n || !path) return -1;
    FILE *f = fopen(path, "wb"); if (!f) return -2;
    fwrite(&n->uid,4,1,f); fwrite(&n->num_layers,4,1,f);
    fwrite(&n->input_dim,4,1,f); fwrite(&n->output_dim,4,1,f);
    for (uint32_t i = 0; i < n->num_layers; i++) {
        fwrite(&n->layers[i].type,4,1,f);
        fwrite(&n->layers[i].uid,4,1,f);
    }
    fclose(f); return 0;
}

BoolNet* boolnet_load(const char *path)
{
    if (!path) return NULL;
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    uint32_t uid, nl, id, od;
    if (fread(&uid,4,1,f)!=1||fread(&nl,4,1,f)!=1||
        fread(&id,4,1,f)!=1||fread(&od,4,1,f)!=1) { fclose(f); return NULL; }
    BoolNet *n = boolnet_create(uid, id, nl ? nl : 1);
    if (!n) { fclose(f); return NULL; }
    for (uint32_t i = 0; i < nl; i++) {
        uint32_t type, luid;
        if (fread(&type,4,1,f)!=1||fread(&luid,4,1,f)!=1)
        { boolnet_destroy(n); fclose(f); return NULL; }
        n->layers[i].type = (LayerType)type;
        n->layers[i].uid = luid;
    }
    n->num_layers = nl; n->output_dim = od;
    fclose(f); return n;
}
