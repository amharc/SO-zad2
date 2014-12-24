// Krzysztof Pszeniczny (kp347208)
#ifndef _COMMON_H
#define _COMMON_H

#define _XOPEN_SOURCE 700
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

enum message_type {
	// Request sent by a client to the server, indicating start of communication
	CHANNEL_REQUEST,

	// Message sent by the server to a polling station, indicating that the station
	// should continue
	CONNECTION_ACCEPTED,

	// Message sent by the server to a polling station, indicating that the station
	// has already started its connection
	CONNECTION_REFUSED,

	// Message sent by the polling station to the server containing the header of
	// the voting results
	RESULTS_START,

	// Message sent by the polling station to the server containg the number of
	// votes cast for a particular candidate
	RESULTS_PARTIAL,

	// Message seny by the polling station to the server indicating that no more
	// RESULTS_PARTIAL messages will follow
	RESULTS_END,

	// Message sent by the server to the polling station indicating that the results
	// were accepted. It also contains the summary of the received results
	RESULTS_OK,

	// Message sent by the client to the server forming a request to deliver a partial
	// report, i.e. for a particular list
	GENERATE_PARTIAL_REPORT,

	// Message sent by the client to the server forming a request to deliver a full
	// report
	GENERATE_FULL_REPORT,

	// Message sent by the server to the client containing the header of the report
	REPORT_HEADER,	

	// Message sent by the server to the client containing the header of the results
	// of one list
	REPORT_LIST_HEADER,

	// Message sent by the server to the client containing the results of a particular
	// candidate
	REPORT_CANDIDATE,

	// Message sent by the server to the client indicating that no REPORT_POSITION
	// message will follow
	REPORT_END
};

struct channel_request_message {
	// PID of the process, will be used as the channel identifier
	pid_t pid;
	// id > 0  -- identifier of the polling station
	// id == 0 -- means that a report is requested
	uint16_t id;
};

// Message sent by the polling station to the server
struct results_start_message {
	// Number of eligible voters
	uint32_t eligible;
	// Number of ballots cast
	uint32_t cast;
};

// Message sent by the polling station to the server
struct results_partial_message {
	uint16_t list;
	uint16_t candidate;
	// Number of votes cast for this particular candidate
	uint32_t count;
};

// Message sent by the server to the polling station
struct results_ok_message {
	// Number of entries processed
	uint32_t entries;
	// Number of valid votes cast
	uint32_t valid;
};

// Message sent by the client wanting to receive a full report
struct generate_partial_report_message {
	uint64_t list;
};

// Message sent by the server to a client
struct report_header_message {
	uint16_t processed_stations;
	uint16_t total_stations;
	uint32_t eligible;
	uint32_t valid_votes;
	uint32_t invalid_votes;
	uint16_t lists;
	uint16_t candidates_per_list;
};

// Message sent by the server to a client
struct report_list_header_message {
	// The list id
	uint16_t list;
	// The total number of the votes cast on this list
	uint32_t total;
};

// Message sent by the server to a client
struct report_candidate_message {
	uint16_t list;
	uint16_t candidate;
	uint32_t votes;
};

struct message {
	long channel;
	enum message_type type;
	union {
		struct channel_request_message channel_request;

		struct results_start_message results_start;
		struct results_partial_message results_partial;
		struct results_ok_message results_ok;

		struct generate_partial_report_message generate_partial_report;
		struct report_header_message report_header;
		struct report_list_header_message report_list_header;
		struct report_candidate_message report_candidate;
	};
};

// The client reads from the READ queue and writes to the WRITE queue
// The server does it the opposite way, for obvious reasons
// The initial requests are sent to a separate queue to avoid deadlock
// caused by the thread limiting
#define REQUEST_KEY 843L
#define WRITE_KEY 1410L
#define READ_KEY 1812L

// Channel on which the server listens for connections
// on the requests queue
#define CHANNEL_REQUEST_CHANNEL 1

#define MSG_SIZE (sizeof(struct message) - sizeof(long))

#endif // _COMMON_H
