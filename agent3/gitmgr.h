#ifndef AGENT3_GITMGR_H
#define AGENT3_GITMGR_H
#include <stdint.h>
int  gitmgr_init(void);
int  gitmgr_commit(uint32_t uid, const char *module_name);
void gitmgr_shutdown(void);
#endif
