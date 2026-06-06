#include <string.h>
#include "vector_ops.h"
void vector_copy(float *dst, const float *src, uint32_t dim) {
    if (dst && src && dim) memcpy(dst, src, (size_t)dim * sizeof(float));
}
void vector_zero(float *vec, uint32_t dim) {
    if (vec && dim) memset(vec, 0, (size_t)dim * sizeof(float));
}
bool vector_is_zero(const float *vec, uint32_t dim) {
    if (!vec) return true;
    for (uint32_t i = 0; i < dim; i++) if (vec[i] != 0.0f) return false;
    return true;
}
