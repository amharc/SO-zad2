// Krzysztof Pszeniczny (kp347208)
#include "common.h"
#include "client-common.h"

_Noreturn void usage(const char *name) {
	fprintf(stderr, "Usage: %s m\n", name);
	exit(1);
}

void perform_handshake(long channel) {
	struct message msg;
	memset(&msg, 0, sizeof(struct message));

	if(-1 == msgrcv(read_queue_id, &msg, MSG_SIZE, channel, 0))
		client_error("Unable to receive reply");

	if(CONNECTION_REFUSED == msg.type) {
		printf("Odmowa dostępu\n");
		exit(-1);
	}
	
	if(CONNECTION_ACCEPTED != msg.type)
		client_error("Invalid server response");
}

int main(int argc, char **argv) {
	if(argc != 2)
		usage(argv[0]);

	int id = atoi(argv[1]);
	long channel = get_channel(id);
	perform_handshake(channel);

	struct message msg;
	memset(&msg, 0, sizeof(msg));

	int eligible, cast;
	scanf("%d %d", &eligible, &cast);

	msg.channel = channel;
	msg.type = RESULTS_START;
	msg.results_start.eligible = eligible;
	msg.results_start.cast = cast;

	if(-1 == msgsnd(write_queue_id, &msg, MSG_SIZE, 0))
		client_error("Unable to send the header");

	int list, candidate, count;
	while(3 == scanf("%d %d %d", &list, &candidate, &count)) {
		msg.channel = channel;
		msg.type = RESULTS_PARTIAL;
		msg.results_partial.list = list;
		msg.results_partial.candidate = candidate;
		msg.results_partial.count = count;

		if(-1 == msgsnd(write_queue_id, &msg, MSG_SIZE, 0))
			client_error("Unable to send partial results");
	}

	msg.channel = channel;
	msg.type = RESULTS_END;
	if(-1 == msgsnd(write_queue_id, &msg, MSG_SIZE, 0))
		client_error("Unable to send end-of-results information");

	if(-1 == msgrcv(read_queue_id, &msg, MSG_SIZE, channel, 0))
		client_error("Unable to receive reply");

	printf("Przetworzonych wpisów: %d\n", msg.results_ok.entries);
	printf("Uprawnionych do głosowania: %d\n", eligible);
	printf("Głosów ważnych: %d\n", msg.results_ok.valid);
	printf("Głosów nieważnych: %d\n", cast - msg.results_ok.valid);
	printf("Frekwencja w lokalu: %.2f%%\n", eligible == 0 ? 100.0 : cast * 100.0 / eligible);
	return 0;
}
