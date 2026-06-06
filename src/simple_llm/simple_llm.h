/**
 * simple_llm.h — Simple Dialogue Model (BoolNet-based LLM)
 *
 * Architecture: Router(128b)→Memory(128c)→Router(128b)→Memory(16c)
 * Task: 5 fixed dialogue pairs (greeting, status, identity, farewell, thanks)
 * See: docs/interact/simple_llm_design.md
 */
#ifndef SIMPLE_LLM_H
#define SIMPLE_LLM_H
#include <stdint.h>

#define SL_VOCAB_SIZE   16    /* input and output vocabulary size */
#define SL_MEM1_CELLS   128   /* context memory cells */
#define SL_MEM2_CELLS   16    /* output memory cells */
#define SL_TRAIN_STEPS  20000
#define SL_BYTE_MAX     (SL_MEM2_CELLS * 255)

/* ---- Dialogue data ---- */
extern const char *sl_input_tokens[SL_VOCAB_SIZE];
extern const char *sl_output_tokens[SL_VOCAB_SIZE];
extern const uint8_t sl_train_pairs[5][2];   /* [pair][0]=input_idx, [1]=output_idx */

/* ---- Network builder ---- */
void* sl_build_network(void);   /* returns BoolNet* */

/* ---- Training ---- */
int sl_train(void *net, const char *model_path);

/* ---- Inference ---- */
int sl_infer(void *net, int token_idx, int *out_idx, uint8_t *out_values);

/* ---- Demo ---- */
int sl_demo(void);

#endif
