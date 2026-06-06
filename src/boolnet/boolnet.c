#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "boolnet.h"
#include "../bool_router/bool_router.h"
#include "../mem_int/mem_int_layer.h"
#include "../conv1d_circular/conv1d_circular.h"
#include "../upsampling/upsampling.h"

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

    /* Header */
    fwrite(&n->uid,4,1,f); fwrite(&n->num_layers,4,1,f);
    fwrite(&n->input_dim,4,1,f); fwrite(&n->output_dim,4,1,f);

    for (uint32_t i = 0; i < n->num_layers; i++) {
        fwrite(&n->layers[i].type,4,1,f);
        fwrite(&n->layers[i].uid,4,1,f);

        /* Save layer data to temp file, then inline it */
        uint32_t data_size = 0;
        long size_pos = ftell(f);
        fwrite(&data_size, 4, 1, f); /* placeholder */

        if (n->layers[i].save_fn) {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "_bn_tmp_%u.bin", i);
            if (n->layers[i].save_fn(n->layers[i].instance, tmp) == 0) {
                FILE *tf = fopen(tmp, "rb");
                if (tf) {
                    fseek(tf, 0, SEEK_END);
                    data_size = (uint32_t)ftell(tf);
                    fseek(tf, 0, SEEK_SET);
                    uint8_t *buf = (uint8_t*)malloc(data_size);
                    if (buf) {
                        fread(buf, 1, data_size, tf);
                        fwrite(buf, 1, data_size, f);
                        free(buf);
                    }
                    fclose(tf);
                }
                remove(tmp);
            }
        }

        /* Backfill actual size */
        long cur = ftell(f);
        fseek(f, size_pos, SEEK_SET);
        fwrite(&data_size, 4, 1, f);
        fseek(f, cur, SEEK_SET);
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

    /* Read inline layer data */
    for (uint32_t i = 0; i < nl; i++) {
        uint32_t data_size;
        if (fread(&data_size, 4, 1, f) != 1) continue;
        if (data_size == 0 || !n->layers[i].save_fn) {
            /* skip data */
            fseek(f, data_size, SEEK_CUR);
            continue;
        }

        /* Write data to temp file, load via layer's load function */
        uint8_t *buf = (uint8_t*)malloc(data_size);
        if (!buf) { fseek(f, data_size, SEEK_CUR); continue; }
        if (fread(buf, 1, data_size, f) != data_size) { free(buf); continue; }

        char tmp[64];
        snprintf(tmp, sizeof(tmp), "_bn_load_%u.bin", i);
        FILE *tf = fopen(tmp, "wb");
        if (tf) {
            fwrite(buf, 1, data_size, tf);
            fclose(tf);
            /* Call type-specific load to restore instance */
            if (n->layers[i].type == LAYER_ROUTER) {
                BoolRouter *r = bool_router_load(tmp);
                if (r) n->layers[i].instance = r;
            } else if (n->layers[i].type == LAYER_MEMORY) {
                MemIntLayer *m = mem_int_load(tmp);
                if (m) n->layers[i].instance = m;
            } else if (n->layers[i].type == LAYER_CONV1D) {
                Conv1D *c = conv1d_load(tmp);
                if (c) n->layers[i].instance = c;
            } else if (n->layers[i].type == LAYER_UPSAMPLE) {
                UpsampleLayer *u = upsample_load(tmp);
                if (u) n->layers[i].instance = u;
            }
        }
        free(buf);
        remove(tmp);
    }
    fclose(f); return n;
}
