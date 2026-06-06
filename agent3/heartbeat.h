#ifndef AGENT3_HEARTBEAT_H
#define AGENT3_HEARTBEAT_H

/* Initialize heartbeat writer. filepath = "./docs/interact/.agent3_heartbeat" */
int  heartbeat_init(const char *filepath);

/* Write current timestamp to the heartbeat file */
void heartbeat_tick(void);

/* Shutdown heartbeat */
void heartbeat_shutdown(void);

#endif /* AGENT3_HEARTBEAT_H */
