#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "boolnet.h"

BoolNet* boolnet_create(LayerUID uid, uint32_t input_dim, uint32_t max_layers) {
    BoolNet *n = (BoolNet*)calloc(1, sizeof(BoolNet));
    if (!n) return NULL;
    n->uid = uid; n->input_dim = input_dim; n->output_dim = input_dim;
    n->capacity = max_layers; n->num_layers = 0;
    n->layers = (BoolNetLayer*)calloc(max_layers, sizeof(BoolNetLayer));
    if (!n->layers) { free(n); return NULL; }
    return n;
}
void boolnet_destroy(BoolNet *n) {
    if (!n) return; free(n->layers); free(n);
}
int boolnet_add_layer(BoolNet *n, LayerType type, LayerUID uid, void *inst,
                      int (*fwd)(void*,const float*,float*),
                      int (*sv)(void*,const char*)) {
    if (!n || n->num_layers >= n->capacity) return -1;
    /* insert sorted by UID */
    uint32_t pos = n->num_layers;
    while (pos > 0 && n->layers[pos-1].uid > uid) {
        n->layers[pos] = n->layers[pos-1]; pos--;
    }
    n->layers[pos].type = type; n->layers[pos].uid = uid;
    n->layers[pos].instance = inst; n->layers[pos].forward = fwd; n->layers[pos].save_fn = sv;
    n->num_layers++;
    return 0;
}
int boolnet_forward(BoolNet *n, const float *input, float *output) {
    if (!n || !input || !output) return -1;
    if (n->num_layers == 0) { memcpy(output, input, n->input_dim * sizeof(float)); return 0; }
    /* double-buffer for in-place propagation */
    float *buf[2];
    buf[0] = (float*)malloc((size_t)n->input_dim * 4 * sizeof(float)); /* generous */
    buf[1] = (float*)malloc((size_t)n->input_dim * 4 * sizeof(float));
    if (!buf[0] || !buf[1]) { free(buf[0]); free(buf[1]); return -2; }
    memcpy(buf[0], input, n->input_dim * sizeof(float));
    int src = 0;
    for (uint32_t i = 0; i < n->num_layers; i++) {
        int rc = n->layers[i].forward(n->layers[i].instance, buf[src], buf[1-src]);
        if (rc) { free(buf[0]); free(buf[1]); return rc; }
        src = 1 - src;
    }
    memcpy(output, buf[src], n->output_dim * sizeof(float));
    free(buf[0]); free(buf[1]);
    return 0;
}
int boolnet_save(const BoolNet *n, const char *path) {
    if (!n || !path) return -1;
    FILE *f = fopen(path, "wb"); if (!f) return -2;
    fwrite(&n->uid,4,1,f); fwrite(&n->num_layers,4,1,f);
    fwrite(&n->input_dim,4,1,f); fwrite(&n->output_dim,4,1,f);
    for (uint32_t i = 0; i < n->num_layers; i++) {
        fwrite(&n->layers[i].type,4,1,f); fwrite(&n->layers[i].uid,4,1,f);
    }
    fclose(f); return 0;
}
BoolNet* boolnet_load(const char *path) {
    if (!path) return NULL;
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    uint32_t uid,nl,id,od;
    if (fread(&uid,4,1,f)!=1||fread(&nl,4,1,f)!=1||fread(&id,4,1,f)!=1||fread(&od,4,1,f)!=1) {fclose(f);return NULL;}
    BoolNet *n = boolnet_create(uid, id, nl ? nl : 1);
    if (!n) {fclose(f);return NULL;}
    for (uint32_t i = 0; i < nl; i++) {
        uint32_t type, luid;
        if (fread(&type,4,1,f)!=1||fread(&luid,4,1,f)!=1) {boolnet_destroy(n);fclose(f);return NULL;}
        n->layers[i].type = (LayerType)type; n->layers[i].uid = luid;
    }
    n->num_layers = nl; n->output_dim = od;
    fclose(f); return n;
}
