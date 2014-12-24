// Krzysztof Pszeniczny (kp347208)
#include "serwer-report.h"

void process_report(long channel) {
	int list;
	struct message msg;
	memset(&msg, 0, sizeof(msg));

	if(-1 == msgrcv(write_queue_id, &msg, MSG_SIZE, channel, 0))
		serv_msg_error("Unable to receive message");

	switch(msg.type) {
		case GENERATE_PARTIAL_REPORT:
			list = msg.generate_partial_report.list - 1;
			break;
		case GENERATE_FULL_REPORT:
			list = -1;
			break;
		default:
			serv_error("Invalid message type");
	}

	check_interrupted();
	msg.channel = channel;
	msg.type = REPORT_HEADER;

	if(-1 == pthread_rwlock_rdlock(&lock))
		serv_error("Unable to acquire rdlock");

	// Everything is copied here, because msgsnd may cause our thread to sleep
	// Note: memory is allocated on stack, which is already allocated for the thread
	msg.report_header.processed_stations = processed_stations;
	msg.report_header.total_stations = M;
	msg.report_header.eligible = eligible;
	msg.report_header.valid_votes = valid_votes;
	msg.report_header.invalid_votes = invalid_votes;
	msg.report_header.lists = L;
	msg.report_header.candidates_per_list = K;

	int copy_results[L][K];
	for(int i = 0; i < L; ++i)
		for(int j = 0; j < K; ++j)
			copy_results[i][j] = results[i][j];

	if(-1 == pthread_rwlock_unlock(&lock))
		serv_error("Unable to release rdlock");

	// Send the data to the client

	check_interrupted();
	if(-1 == msgsnd(read_queue_id, &msg, MSG_SIZE, 0))
		serv_msg_error("Unable to send report header");

	for(int i = 0; i < L; ++i)
		if(-1 == list || i == list) {
			msg.channel = channel;
			msg.type = REPORT_LIST_HEADER;
			msg.report_list_header.list = i + 1;
			msg.report_list_header.total = 0;
			for(int j = 0; j < K; ++j) {
				msg.report_list_header.total += copy_results[i][j];
			}

			check_interrupted();
			if(-1 == msgsnd(read_queue_id, &msg, MSG_SIZE, 0))
				serv_msg_error("Unable to send list header");

			for(int j = 0; j < K; ++j) {
				msg.channel = channel;
				msg.type = REPORT_CANDIDATE;
				msg.report_candidate.list = i + 1;
				msg.report_candidate.candidate = j + 1;
				msg.report_candidate.votes = copy_results[i][j];

				check_interrupted();
				if(-1 == msgsnd(read_queue_id, &msg, MSG_SIZE, 0))
					serv_msg_error("Unable to send candidate's results");
			}
		}

	// End the report
	msg.channel = channel;
	msg.type = REPORT_END;

	check_interrupted();
	if(-1 == msgsnd(read_queue_id, &msg, MSG_SIZE, 0))
		serv_msg_error("Unable to send end-of-report message");
}
