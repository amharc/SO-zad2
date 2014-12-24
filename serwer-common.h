// Krzysztof Pszeniczny (kp347208)
#ifndef _SERWER_COMMON_H
#define _SERWER_COMMON_H

#include "common.h"

#define MAX_THREADS 100

extern int L, K, M;
extern volatile int alive_threads;

extern volatile int processed_stations;
extern volatile int valid_votes;
extern volatile int invalid_votes;
extern volatile int eligible;
extern volatile int **results;
extern volatile bool *stations;

extern int request_queue_id;
extern int read_queue_id;
extern int write_queue_id;

// Set to true when the server is about to exit
extern volatile bool interrupted;

// Thread attributes
extern pthread_attr_t attr;
// Used to lock access to alive_threads
extern pthread_mutex_t counter_mutex;
// Used by the thread which is handling the server shutdown
// to wait for other threads
extern pthread_cond_t finish_cond;
// Used to wait for a new thread slot (due to the limit on thread count)
extern pthread_cond_t slot_cond;
// Used to lock access to all votes-related variables
extern pthread_rwlock_t lock;

// Exits the server due to an error
_Noreturn void serv_error(const char *fmt, ...);

// Initializes the server
void init(void);

// Performs the ultimate cleanup of the server
// Only the main thread will perform cleanup or exit the
// server
void cleanup(void);

// Exits the thread
_Noreturn void exit_thread(void);

// Exits the server.
_Noreturn void exit_server(int retcode);

// Block (or unblock) signals
void block_signals(bool block);

void lock_mutex(pthread_mutex_t*);
void unlock_mutex(pthread_mutex_t*);

// Checks if interrupted is true, if yes, then exits the thread
void check_interrupted(void);

#define serv_msg_error(...) { check_interrupted(); serv_error(__VA_ARGS__); }
#endif
