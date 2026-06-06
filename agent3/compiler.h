#ifndef AGENT3_COMPILER_H
#define AGENT3_COMPILER_H
#include <stdint.h>
/* Compile via make in test dir. Returns 0 on success. Captures output into *output (caller frees). */
int compile_module(uint32_t uid, char **output, uint32_t *duration_ms);
#endif
