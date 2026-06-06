#ifndef VECTOR_OPS_H
#define VECTOR_OPS_H
#include <stdint.h>
#include <stdbool.h>
void vector_copy(float *dst, const float *src, uint32_t dim);
void vector_zero(float *vec, uint32_t dim);
bool vector_is_zero(const float *vec, uint32_t dim);
#endif
