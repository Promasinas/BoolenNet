#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mem_int_layer.h"

static inline uint64_t cell_read(const MemIntLayer *l, uint32_t i) {
    switch(l->precision){case 0:return((uint8_t*)l->cells)[i];case 1:return((uint16_t*)l->cells)[i];case 2:return((uint32_t*)l->cells)[i];case 3:return((uint64_t*)l->cells)[i];default:return 0;}
}
static inline void cell_write(const MemIntLayer *l, uint32_t i, uint64_t v) {
    switch(l->precision){case 0:((uint8_t*)l->cells)[i]=(uint8_t)v;break;case 1:((uint16_t*)l->cells)[i]=(uint16_t)v;break;case 2:((uint32_t*)l->cells)[i]=(uint32_t)v;break;case 3:((uint64_t*)l->cells)[i]=v;break;}
}
static inline size_t csize(uint8_t p) { const size_t s[]={1,2,4,8}; return (p<4)?s[p]:0; }

MemIntLayer* mem_int_create(LayerUID uid, uint8_t prec, uint32_t len, uint64_t maxv, uint64_t decay) {
    if (!len || prec>3) return NULL;
    size_t cs = csize(prec); if (!cs) return NULL;
    MemIntLayer *l = (MemIntLayer*)calloc(1,sizeof(MemIntLayer));
    if (!l) return NULL;
    uint64_t pmax = (prec==0?UINT8_MAX:prec==1?UINT16_MAX:prec==2?UINT32_MAX:UINT64_MAX);
    l->uid=uid; l->precision=prec; l->length=len; l->max_value=(maxv>pmax)?pmax:maxv; l->decay=decay;
    l->cells=calloc(len,cs);
    if(!l->cells){free(l);return NULL;}
    return l;
}
void mem_int_destroy(MemIntLayer *l) { if(l){free(l->cells);free(l);} }
void mem_int_forward(MemIntLayer *l, const uint8_t *sig) {
    if(!l||!sig) return;
    for(uint32_t i=0;i<l->length;i++) {
        if(sig[i]) cell_write(l,i,l->max_value);
        else { uint64_t v=cell_read(l,i); cell_write(l,i,(v>=l->decay)?v-l->decay:0); }
    }
}
void mem_int_query(const MemIntLayer *l, uint8_t *mask) {
    if(!l||!mask) return;
    for(uint32_t i=0;i<l->length;i++) mask[i]=(cell_read(l,i)>0)?1:0;
}
int mem_int_save(const MemIntLayer *l, const char *fp) {
    if(!l||!fp) return ML_ERR_NULL_PARAM;
    FILE *f=fopen(fp,"wb"); if(!f) return ML_ERR_FILE_OPEN;
    uint32_t u=l->uid,len=l->length; uint8_t p=l->precision; uint64_t mv=l->max_value,d=l->decay;
    fwrite(&u,4,1,f); fwrite(&p,1,1,f); fwrite(&len,4,1,f); fwrite(&mv,8,1,f); fwrite(&d,8,1,f);
    size_t w=fwrite(l->cells,csize(l->precision),l->length,f); fclose(f);
    return (w==l->length)?ML_OK:ML_ERR_FILE_WRITE;
}
MemIntLayer* mem_int_load(const char *fp) {
    if(!fp) return NULL;
    FILE *f = fopen(fp, "rb");
    if(!f) return NULL;
    uint32_t u,len; uint8_t p; uint64_t mv,d;
    if(fread(&u,4,1,f)!=1||fread(&p,1,1,f)!=1||fread(&len,4,1,f)!=1||fread(&mv,8,1,f)!=1||fread(&d,8,1,f)!=1) {fclose(f);return NULL;}
    if(!len||p>3){fclose(f);return NULL;}
    MemIntLayer *l=mem_int_create(u,p,len,mv,d); if(!l){fclose(f);return NULL;}
    if(fread(l->cells,csize(p),len,f)!=len){mem_int_destroy(l);fclose(f);return NULL;}
    fclose(f); return l;
}
void mem_int_reset(MemIntLayer *l) {
    if (!l) return;
    memset(l->cells, 0, l->length * csize(l->precision));
}
int mem_int_all_zero(const MemIntLayer *l) {
    if(!l) return 1;
    for(uint32_t i=0;i<l->length;i++) if(cell_read(l,i)!=0) return 0;
    return 1;
}
int mem_int_verify_roundtrip(const MemIntLayer *l) {
    if(!l) return -1;
    const char *tf="_mem_int_rt.bin";
    if(mem_int_save(l,tf)!=ML_OK) return -2;
    MemIntLayer *ld=mem_int_load(tf); if(!ld){remove(tf);return -3;}
    int mismatch=0;
    if(l->uid!=ld->uid||l->precision!=ld->precision||l->length!=ld->length||l->max_value!=ld->max_value||l->decay!=ld->decay) mismatch=1;
    if(!mismatch && memcmp(l->cells,ld->cells,l->length*csize(l->precision))!=0) mismatch=2;
    mem_int_destroy(ld); remove(tf); return mismatch;
}

/* BoolNet layer adaptor: input bytes → trigger signal → forward → query mask → output */
int mem_int_forward_layer(void *inst, const uint8_t *in, uint8_t *out)
{
    MemIntLayer *l = (MemIntLayer*)inst;
    if (!l || !in || !out) return -1;

    /* Treat input bytes directly as router signal bitmask */
    uint32_t n = l->length;
    uint8_t *mask = (uint8_t*)malloc(n);
    if (!mask) return -2;

    mem_int_forward(l, in);    /* in = router signal */
    mem_int_query(l, mask);    /* mask = trigger state */
    memcpy(out, mask, n);      /* output = trigger mask bytes */

    free(mask);
    return 0;
}

/* Output adaptor: reads actual cell values (0-255), not binary trigger mask.
 * This preserves value differences (e.g. 254 vs 255) for classification. */
int mem_int_forward_output(void *inst, const uint8_t *in, uint8_t *out)
{
    MemIntLayer *l = (MemIntLayer*)inst;
    if (!l || !in || !out) return -1;

    /* Forward with signal, then read raw cell values */
    mem_int_forward(l, in);

    for (uint32_t i = 0; i < l->length; i++) {
        uint64_t v = cell_read(l, i);
        out[i] = (v > 255) ? 255 : (uint8_t)v;
    }
    return 0;
}
