// Krzysztof Pszeniczny (kp347208)
#include "client-common.h"

int request_queue_id;
int read_queue_id;
int write_queue_id;

_Noreturn void client_error(const char *fmt, ...) {
	va_list fmt_args;
	fprintf(stderr, "Client error: ");
	va_start(fmt_args, fmt);
	vfprintf(stderr, fmt, fmt_args);
	va_end(fmt_args);
	fprintf(stderr, " (%d; %s)\n", errno, strerror(errno));
	exit(1);
}

long get_channel(int id) {
	struct message msg;
	memset(&msg, 0, sizeof(struct message));
	pid_t pid = getpid();
	msg.channel = CHANNEL_REQUEST_CHANNEL;
	msg.type = CHANNEL_REQUEST;
	msg.channel_request.pid = pid;
	msg.channel_request.id = id;

	if(-1 == (request_queue_id = msgget(REQUEST_KEY, 0666)))
		client_error("Unable to open the read queue");

	if(-1 == (read_queue_id = msgget(READ_KEY, 0666)))
		client_error("Unable to open the read queue");

	if(-1 == (write_queue_id = msgget(WRITE_KEY, 0666)))
		client_error("Unable to open the write queue");

	if(-1 == msgsnd(request_queue_id, &msg, MSG_SIZE, 0))
		client_error("unable to send the channel request");

	return pid;
}
