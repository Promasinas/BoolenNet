#ifndef AGENT3_TESTDIR_H
#define AGENT3_TESTDIR_H
#include <stdint.h>
int  testdir_create(uint32_t uid, const char *module_name);
int  testdir_generate_framework(uint32_t uid, const char *module_name);
#endif
