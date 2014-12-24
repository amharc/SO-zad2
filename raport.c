// Krzysztof Pszeniczny (kp347208)
#include "common.h"
#include "client-common.h"

_Noreturn void usage(const char *name) {
	fprintf(stderr, "Usage: %s [l]", name);
	exit(1);
}

int main(int argc, char **argv) {
	if(argc > 2)
		usage(argv[0]);

	long channel = get_channel(0);
	struct message msg;
	memset(&msg, 0, sizeof(struct message));

	msg.channel = channel;
	if(argc == 1)
		msg.type = GENERATE_FULL_REPORT;
	else {
		msg.type = GENERATE_PARTIAL_REPORT;
		msg.generate_partial_report.list = atoi(argv[1]);
	}

	if(-1 == msgsnd(write_queue_id, &msg, MSG_SIZE, 0))
		client_error("unable to send the request");

	if(-1 == msgrcv(read_queue_id, &msg, MSG_SIZE, channel, 0))
		client_error("Unable to receive the header");

	printf("Przetworzonych komisji: %" PRIu16 " / %" PRIu16 "\n",
			msg.report_header.processed_stations,
			msg.report_header.total_stations);

	printf("Uprawnionych do głosowania: %" PRIu32 "\n", msg.report_header.eligible);
	printf("Głosów ważnych: %" PRIu32 "\n", msg.report_header.valid_votes);
	printf("Głosów nieważnych: %" PRIu32 "\n", msg.report_header.invalid_votes);
	double turnout = msg.report_header.eligible == 0 ? 100.0
		: (msg.report_header.valid_votes + msg.report_header.invalid_votes) * 100.0 / msg.report_header.eligible;
	printf("Frekwencja: %.2lf%%\n", turnout);
	printf("Wyniki poszczególnych list:");

	while(true) {
		if(-1 == msgrcv(read_queue_id, &msg, MSG_SIZE, channel, 0))
			client_error("Unable to receive results");

		switch(msg.type) {
			case REPORT_END:
				puts("");
				return 0;
			case REPORT_LIST_HEADER:
				printf("\n%hd %d", msg.report_list_header.list, msg.report_list_header.total);
				break;
			case REPORT_CANDIDATE:
				printf(" %d", msg.report_candidate.votes);
				break;
			default:
				client_error("Invalid server response");
		}
	}
	return 0;
}
