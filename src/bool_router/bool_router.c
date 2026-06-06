#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "bool_router.h"

BoolRouter* bool_router_create(LayerUID uid, uint32_t num_bits, const uint8_t *bits)
{
    if (!num_bits) return NULL;
    BoolRouter *r = (BoolRouter*)calloc(1, sizeof(BoolRouter));
    if (!r) return NULL;
    r->uid = uid;
    r->num_bits = num_bits;
    uint32_t num_bytes = (num_bits + 7) / 8;
    r->bits = (uint8_t*)calloc(num_bytes, 1);
    if (!r->bits) { free(r); return NULL; }
    /* Only copy what was provided; calloc already zero-filled the rest */
    if (bits) {
        uint32_t provided_bytes = (num_bits + 7) / 8;
        /* Safer: copy exactly num_bytes bytes; caller must provide full array */
        memcpy(r->bits, bits, num_bytes);
    }
    return r;
}

void bool_router_destroy(BoolRouter *r)
{
    if (!r) return;
    free(r->bits);
    free(r);
}

void bool_router_forward(const BoolRouter *r, const uint8_t *input, uint8_t *output)
{
    if (!r || !input || !output) return;
    uint32_t bytes = (r->num_bits + 7) / 8;
    for (uint32_t i = 0; i < bytes; i++)
        output[i] = input[i] ^ r->bits[i];
    /* Mask trailing bits */
    uint32_t rem = r->num_bits & 7;
    if (rem) output[bytes-1] &= (uint8_t)((1u << rem) - 1);
}

/* Tsetlin: flip one bit, return 0 on success */
int bool_router_flip_bit(BoolRouter *r, uint32_t bit_idx)
{
    if (!r || bit_idx >= r->num_bits) return -1;
    uint32_t byte_idx = bit_idx / 8;
    uint8_t  bit_mask = (uint8_t)(1u << (bit_idx & 7));
    r->bits[byte_idx] ^= bit_mask;
    return 0;
}

/* BoolNet layer adaptor: uint8_t[N] → XOR with router bits → uint8_t[N] */
int bool_router_forward_layer(void *inst, const uint8_t *in, uint8_t *out)
{
    BoolRouter *r = (BoolRouter*)inst;
    if (!r || !in || !out) return -1;
    bool_router_forward(r, in, out);
    return 0;
}

int bool_router_save(const BoolRouter *r, const char *path)
{
    if (!r || !path) return -1;
    FILE *f = fopen(path, "wb"); if (!f) return -2;
    uint32_t bytes = (r->num_bits + 7) / 8;
    fwrite(&r->uid, 4, 1, f); fwrite(&r->num_bits, 4, 1, f);
    fwrite(r->bits, 1, bytes, f);
    fclose(f); return 0;
}

BoolRouter* bool_router_load(const char *path)
{
    if (!path) return NULL;
    FILE *f = fopen(path, "rb"); if (!f) return NULL;
    uint32_t uid, nb;
    if (fread(&uid,4,1,f)!=1 || fread(&nb,4,1,f)!=1) { fclose(f); return NULL; }
    uint32_t bytes = (nb + 7) / 8;
    uint8_t *bits = (uint8_t*)malloc(bytes);
    if (!bits) { fclose(f); return NULL; }
    if (fread(bits, 1, bytes, f) != bytes) { free(bits); fclose(f); return NULL; }
    fclose(f);
    BoolRouter *r = bool_router_create(uid, nb, bits);
    free(bits);
    return r;
}
