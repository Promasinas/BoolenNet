/**
 * train_test.c — Pure Boolean Tsetlin Training
 *
 * Task: Learn to route input FF→byte0 to trigger memory cell 0,
 *       while suppressing other bytes. The router must learn
 *       which bits to flip to achieve the desired memory output.
 *
 * Network: Router(32bit) → Memory(4 cells, uint8, max=255, decay=0)
 *   decay=0 means once triggered, cell stays triggered forever.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/boolnet/boolnet.h"
#include "../src/bool_router/bool_router.h"
#include "../src/mem_int/mem_int_layer.h"
#include "../src/tsetlin_train/tsetlin_train.h"

#define N 4
#define STEPS 10000

int main(void)
{
    srand((unsigned)time(NULL));
    printf("=== BoolNet Boolean Training ===\n\n");

    /* Build: Router(32bit, identity init) → Memory(4 cell, decay=0) */
    BoolNet *net = boolnet_create(1, N, 2);
    uint8_t rb[4] = {0x00, 0x00, 0x00, 0x00};
    BoolRouter *r = bool_router_create(1, 32, rb);
    boolnet_add_layer(net, LAYER_ROUTER, 1, r,
        bool_router_forward_layer, (int(*)(void*,const char*))bool_router_save);

    /* decay=0: once triggered, cell stays at max_value permanently */
    MemIntLayer *m = mem_int_create(2, ML_PRECISION_UINT8, N, 255, 0);
    boolnet_add_layer(net, LAYER_MEMORY, 2, m,
        mem_int_forward_layer, (int(*)(void*,const char*))mem_int_save);

    printf("Net: %u layers | decay=0 (persistent trigger)\n\n", net->num_layers);

    /* Task: input=ALL_0x01 → target=only cell0 triggered.
     * Each byte has exactly 1 bit set → flipping that bit → byte becomes 0 → no trigger.
     * Router must learn: bytes 1,2,3 XOR 0x01 → 0x00 (zero out), byte 0 stays. */
    uint8_t in[N] = {0x01, 0x01, 0x01, 0x01};
    uint8_t tg[N] = {0x01, 0x00, 0x00, 0x00};

    /* Verify initial output */
    uint8_t out[N];
    mem_int_reset(m);
    boolnet_forward(net, in, out);
    printf("Initial: in=[%02X %02X %02X %02X] → out=[%02X %02X %02X %02X]",
           in[0],in[1],in[2],in[3], out[0],out[1],out[2],out[3]);
    printf("  %s\n\n", memcmp(out,tg,N)?"(needs training)":"(already correct!)");

    /* Train */
    TsetlinTrainer *t = tsetlin_create(net, 3000, 1020);
    printf("Training %d steps...\n", STEPS);

    for (int s = 0; s < STEPS; s++) {
        int rc = tsetlin_train_step(t, in, tg);
        if (rc == -2) { printf("Stopped at step %d\n", s+1); break; }

        if ((s+1) % 1000 == 0) {
            uint32_t st, ac; int32_t br;
            { uint32_t _ep; int32_t _bv; tsetlin_get_stats(t, &st, &ac, &br, &_ep, &_bv); }
            mem_int_reset(m);
            boolnet_forward(net, in, out);
            int ok = !memcmp(out, tg, N);
            printf("  Step %5d: out=[%02X %02X %02X %02X] %s  acc=%u  best=%d\n",
                   s+1, out[0],out[1],out[2],out[3],
                   ok?"OK":"--", ac, br);
        }
    }

    /* Final */
    mem_int_reset(m);
    boolnet_forward(net, in, out);
    printf("\nFinal: out=[%02X %02X %02X %02X]  target=[%02X %02X %02X %02X]  %s\n",
           out[0],out[1],out[2],out[3], tg[0],tg[1],tg[2],tg[3],
           memcmp(out,tg,N)?"NOT LEARNED":"LEARNED!");

    uint32_t st, ac; int32_t br;
    { uint32_t _ep; int32_t _bv; tsetlin_get_stats(t, &st, &ac, &br, &_ep, &_bv); }
    printf("Steps:%u  Accepted:%u (%.0f%%)  Best reward:%d\n",
           st, ac, st?100.0f*ac/st:0, br);
    printf("Router bits: [%02X %02X %02X %02X]\n",
           r->bits[0], r->bits[1], r->bits[2], r->bits[3]);

    tsetlin_destroy(t); boolnet_destroy(net);
    bool_router_destroy(r); mem_int_destroy(m);
    return 0;
}
