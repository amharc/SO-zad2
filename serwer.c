// Krzysztof Pszeniczny (kp347208)
#include "common.h"
#include "serwer-common.h"
#include "serwer-report.h"
#include "serwer-results.h"

struct thread_param {
	struct message msg;
	bool allowed;
};

void refuse_connection(long channel) {
	struct message msg;
	memset(&msg, 0, sizeof(struct message));
	msg.channel = channel;
	msg.type = CONNECTION_REFUSED;

	check_interrupted();
	if(-1 == msgsnd(read_queue_id, &msg, MSG_SIZE, 0))
		serv_msg_error("Unable to send to %ld", channel);
}

void accept_connection(long channel) {
	struct message msg;
	memset(&msg, 0, sizeof(struct message));
	msg.channel = channel;
	msg.type = CONNECTION_ACCEPTED;

	check_interrupted();
	if(-1 == msgsnd(read_queue_id, &msg, MSG_SIZE, 0))
		serv_msg_error("Unable to send to %ld", channel);
}

_Noreturn void* thread_start(void *ptr) {
	struct message msg = ((struct thread_param*) ptr)->msg;
	bool allowed = ((struct thread_param*) ptr)->allowed;
	long channel = msg.channel_request.pid;

	free(ptr);

	check_interrupted();

	if(msg.channel_request.id > 0) {
		if(!allowed)
			refuse_connection(channel);
		else {
			accept_connection(channel);
			process_results(channel);
		}
	}
	else
		process_report(channel);

	exit_thread();
}

void run(void) {
	int err;
	while(!interrupted) {
		pthread_t thread;
		struct message msg;

		if(-1 == msgrcv(request_queue_id, &msg, MSG_SIZE, CHANNEL_REQUEST_CHANNEL, 0))
			serv_msg_error("Unable to receive a message");

		if(CHANNEL_REQUEST != msg.type)
			serv_error("Invalid message type");

		bool allowed = true;
		if(msg.channel_request.id > 0) {
			if(!stations[msg.channel_request.id - 1])
				stations[msg.channel_request.id - 1] = true;
			else { // Another connection from the same polling station
				allowed = false;
			}
		}

		// Get a new thread slot
		lock_mutex(&counter_mutex);
		while(MAX_THREADS == alive_threads) {
			if(-1 == (err = pthread_cond_wait(&slot_cond, &counter_mutex)))
				serv_error("Unable to wait on slot_cond: %d", err);
		}

		// Mask signals while creating a new thread (as it will inherit the mask)
		block_signals(true);
		struct thread_param *param = malloc(sizeof(struct thread_param));
		if(NULL == param)
			serv_error("Unable to allocate memory");
		param->allowed = allowed;
		param->msg = msg;
		err = pthread_create(&thread, &attr, &thread_start, param);
		if(-1 != err)
			++alive_threads;
		unlock_mutex(&counter_mutex);
		block_signals(false);

		if(-1 == err)
			serv_error("Unable to spawn worker thread: %d", err);
	}
}

_Noreturn void usage(const char *name) {
	fprintf(stderr, "Usage: %s L K M\n", name);
	exit(1);
}

int main(int argc, char **argv) {
	static_assert(sizeof(pid_t) <= sizeof(long),
			"A POSIX-compliant implementation should have sizeof(pid_t) <= sizeof(long)");

	if(argc != 4)
		usage(argv[0]);

	L = atoi(argv[1]);
	K = atoi(argv[2]);
	M = atoi(argv[3]);

	if(L < 0 || K < 0 || M < 0)
		usage(argv[0]);

	init();
	run();
	exit_server(0);
}
