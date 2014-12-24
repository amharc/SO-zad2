// Krzysztof Pszeniczny (kp347208)
#include "serwer-results.h"

void process_results(long channel) {
	struct message msg;
	memset(&msg, 0, sizeof(struct message));

	check_interrupted();
	if(-1 == msgrcv(write_queue_id, &msg, MSG_SIZE, channel, 0))
		serv_msg_error("Unable to receive message");

	if(RESULTS_START != msg.type)
		serv_error("Invalid message");

	// Note: memory is allocated on stack, which is already allocated for the thread
	int this_eligible = msg.results_start.eligible;
	int this_cast = msg.results_start.cast;
	int this_valid = 0;
	int this_votes[L][K];

	memset(this_votes, 0, sizeof(this_votes));

	int processed = 0;

	while(!interrupted) {
		// Receive partial results from the polling station
		if(-1 == msgrcv(write_queue_id, &msg, MSG_SIZE, channel, 0))
			serv_msg_error("Unable to receive message");

		check_interrupted();

		if(RESULTS_END == msg.type)
			break;

		if(RESULTS_PARTIAL != msg.type)
			serv_error("Invalid message type");

		struct results_partial_message part = msg.results_partial;
		++processed;

		this_votes[part.list - 1][part.candidate - 1] = part.count;
		this_valid += part.count;
	}

	check_interrupted();

	// Commit the changes
	if(-1 == pthread_rwlock_wrlock(&lock))
		serv_error("Unable to acquire wrlock");

	processed_stations++;
	valid_votes += this_valid;
	invalid_votes += this_cast - this_valid;
	eligible += this_eligible;
	for(int i = 0; i < L; ++i)
		for(int j = 0; j < K; ++j)
			results[i][j] += this_votes[i][j];

	if(-1 == pthread_rwlock_unlock(&lock))
		serv_error("Unable to release wrlock");

	msg.channel = channel;
	msg.type = RESULTS_OK;
	msg.results_ok.entries = processed;
	msg.results_ok.valid = this_valid;

	check_interrupted();
	if(-1 == msgsnd(read_queue_id, &msg, MSG_SIZE, 0))
		serv_msg_error("Unable to send response");
}
