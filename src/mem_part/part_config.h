#ifndef PART_CONFIG_H
#define PART_CONFIG_H
#include "part_types.h"
int  part_config_init(uint32_t P, uint32_t D);
void part_config_reset(void);
void part_config_get(part_config_t *out);
#endif
