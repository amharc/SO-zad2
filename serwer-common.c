// Krzysztof Pszeniczny (kp347208)
#include "serwer-common.h"

int L, K, M;
volatile int processed_stations;

volatile int alive_threads;
volatile int valid_votes;
volatile int invalid_votes;
volatile int eligible;
volatile int **results;
volatile bool *stations;

int request_queue_id;
int read_queue_id;
int write_queue_id;

volatile bool interrupted;

pthread_attr_t attr;
pthread_mutex_t mutex;
pthread_mutex_t counter_mutex;
pthread_cond_t finish_cond;
pthread_cond_t slot_cond;
pthread_rwlock_t lock;

pthread_t main_thread;
sigset_t block_mask;

_Noreturn void serv_error(const char *fmt, ...) {
	va_list fmt_args;
	fprintf(stderr, "Server error: ");
	va_start(fmt_args, fmt);
	vfprintf(stderr, fmt, fmt_args);
	va_end(fmt_args);
	fprintf(stderr, " (%d; %s)\n", errno, strerror(errno));

	if(interrupted) {
		fprintf(stderr, "An error has occured during error handling. Exiting");
		quick_exit(EXIT_FAILURE);
	}

	interrupted = true;
	// If an error has occurred in the main thread, call the signal handler
	if(pthread_equal(main_thread, pthread_self()))
		exit_server(interrupted);
	else {
		// Otherwise, report the error to the main thread
		if(-1 == kill(getpid(), SIGINT)) {
			fprintf(stderr, "Fatal error. Unable to kill\n");
			quick_exit(EXIT_FAILURE);;
		}
		else
			exit_thread();
	}
}

void block_signals(bool block) {
	int err;

	if(-1 == (err = pthread_sigmask(block ? SIG_BLOCK : SIG_UNBLOCK, &block_mask, NULL)))
		serv_error("Unable to %sblock signals", block ? "" : "un");
}

void lock_mutex(pthread_mutex_t *m) {
	int err;
	if(0 != (err = pthread_mutex_lock(m)))
		serv_error("Unable to lock mutex: %d", err);
}

void unlock_mutex(pthread_mutex_t *m) {
	int err;
	if(0 != (err = pthread_mutex_unlock(m)))
		serv_error("Unable to unlock mutex: %d", err);
}

_Noreturn void exit_thread(void) {
	int err;

	lock_mutex(&counter_mutex);
	--alive_threads;

	if(0 == alive_threads) {
		unlock_mutex(&counter_mutex);
		if(0 != (err = pthread_cond_signal(&finish_cond)))
			serv_error("Unable to signal on finish_cond: %d", err);
	}
	else
		unlock_mutex(&counter_mutex);

	// Notify the server that it may spawn another thread
	if(0 != (err = pthread_cond_signal(&slot_cond)))
		serv_error("Unable to signal on slot_cond: %d", err);

	pthread_exit(NULL);
}

void cleanup_ipc(void) {
	if(-1 == msgctl(write_queue_id, IPC_RMID, 0))
		serv_error("Unable to remove the write queue");

	if(-1 == msgctl(read_queue_id, IPC_RMID, 0))
		serv_error("Unable to remove the read queue");

	if(-1 == msgctl(request_queue_id, IPC_RMID, 0))
		serv_error("Unable to remove the read queue");
}

_Noreturn void exit_server(int retcode) {
	int err;
	interrupted = true;
	cleanup_ipc();

	lock_mutex(&counter_mutex);

	while(alive_threads > 0)
		if(0 != (err = pthread_cond_wait(&finish_cond, &counter_mutex)))
			serv_error("Unable to wait on finish_cond: %d", err);

	unlock_mutex(&counter_mutex);
	
	// mutexes, conditional variables and rwlocks are not destroyed,
	// because the main thread may possiby be blocked on it while handling
	// the signal and destroying mutexes/conds/rwlocks while they are used
	// results is undefined behaviour. Nevertheless, from now on we are sure
	// to have just one thread

	if(results)
		for(int i = 0; i < L; ++i)
			free((void *) results[i]);
	free((void*) results);
	free((void*) stations);
	exit(retcode ? EXIT_FAILURE : EXIT_SUCCESS);
}

_Noreturn void signal_handler() {
	// If the server is commiting suicide (i.e. an error has occurred)
	// interrupted == true BEFORE entering the signal handler, so the
	// exit code will be set accordingly
	exit_server(interrupted);
}

void init() {
	int err;
	struct sigaction action;
	sigset_t empty_mask;

	if(-1 == sigfillset(&block_mask))
		serv_error("Unable to fill block mask");

	int do_not_block[] = {SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGKILL, SIGSTOP};

	for(size_t i = 0; i < sizeof(do_not_block) / sizeof(int); ++i)
		if(-1 == sigdelset(&block_mask, do_not_block[i]))
			serv_error("Unable to remove %d (%s) from the block mask", i, strsignal(i));

	main_thread = pthread_self();
	if(-1 == sigemptyset(&empty_mask))
		serv_error("Unable to clear block mask");

	action.sa_handler = signal_handler;
	action.sa_mask = empty_mask;
	action.sa_flags = SA_RESETHAND;

	if(-1 == sigaction(SIGINT, &action, 0))
		serv_error("Unable to set signal handler");

	if(NULL == (stations = calloc(M, sizeof(bool))))
		serv_error("Unable to allocate memory");

	if(NULL == (results = calloc(L, sizeof(int*))))
		serv_error("Unable to allocate memory");

	for(int i = 0; i < L; ++i)
		if(NULL == (results[i] = calloc(K, sizeof(int))))
			serv_error("Unable to allocate memory");

	if(0 != (err = pthread_attr_init(&attr)))
		serv_error("Unable to initialize attributes: %d", err);

	if(0 != (err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)))
		serv_error("Unable to set detach state: %d", err);

	if(-1 == (request_queue_id = msgget(REQUEST_KEY, 0666 | IPC_CREAT | IPC_EXCL)))
		serv_error("Unable to create the request queue");

	if(-1 == (read_queue_id = msgget(READ_KEY, 0666 | IPC_CREAT | IPC_EXCL)))
		serv_error("Unable to create the read queue");

	if(-1 == (write_queue_id = msgget(WRITE_KEY, 0666 | IPC_CREAT | IPC_EXCL)))
		serv_error("Unable to create the write queue");

	if(0 != (err = pthread_mutex_init(&counter_mutex, NULL)))
		serv_error("Unable to initialize mutex: %d", err);

	if(0 != (err = pthread_cond_init(&finish_cond, NULL)))
		serv_error("Unable to initialize conditional variable: %d", err);

	if(0 != (err = pthread_cond_init(&slot_cond, NULL)))
		serv_error("Unable to initialize conditional variable: %d", err);

	if(0 != (err = pthread_rwlock_init(&lock, NULL)))
		serv_error("Unable to initialize rwlock: %d", err);
}

void check_interrupted() {
	if(interrupted)
		exit_thread();
}
