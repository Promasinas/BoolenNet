#ifndef AGENT3_DETECTOR_H
#define AGENT3_DETECTOR_H

#include "agent3.h"

/* Scan ./src/ for .c/.h files with changes since last poll.
   Populates modules[] with detected modules (up to max_count).
   Returns number of detected modules.
   Handles empty directory gracefully (returns 0). */
int scan_src_dir(const char *src_dir, TestModule *modules, int max_count);

#endif /* AGENT3_DETECTOR_H */
