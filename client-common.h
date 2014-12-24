// Krzysztof Pszeniczny (kp347208)
#ifndef _CLIENT_COMMON_H
#define _CLIENT_COMMON_H

#include "common.h"

extern int request_queue_id;
extern int read_queue_id;
extern int write_queue_id;

// Exits the client due to an error
_Noreturn void client_error(const char *fmt, ...);

// Returns the channel the client should use 
// (In the current implementation: its pid)
long get_channel(int id);

#endif // _CLIENT_COMMON_H
