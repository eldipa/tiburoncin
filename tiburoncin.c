#define _POSIX_C_SOURCE 200112L

#include <sys/select.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hexdump.h"
#include "endpoint.h"
#include "socket.h"
#include "cmdline.h"
#include "circular_buffer.h"

#include "signal.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int passthrough(struct endpoint *ep_producer, struct endpoint *ep_consumer,
		fd_set *rfds, fd_set *wfds,
		struct circular_buffer_t *b,
		struct hexdump *hd) {
	int producer = ep_producer->fd;
	int consumer = ep_consumer->fd;
	int s;

	if (FD_ISSET(producer, rfds)) {	 // ready to produce
		EINTR_RETRY(read(producer, &b->buf[b->head], circular_buffer_get_free(b)));

		if (s < 0) {
			/*
			 * Despite that we use some sort of select/poll multiplexer
			 * the read/write it could block.
			 *
			 * For example, if the fd is a socket and it receives data,
			 * that would mark it "ready for reading" but if the packet
			 * received is corrupted, it will be discarded and the read call will
			 * block because there is not more data.
			 *
			 * To workaround this, the fd must have the O_NONBLOCK flag.
			 * See select(2).
			 * */
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				FD_CLR(producer, rfds);
				goto read_would_block;
			}

			return -1;
		}
		else if (s == 0) {
			/* ack to the other end that we received the shutdown */
			partial_shutdown(ep_producer, SHUT_RD);
			hexdump_shutdown_print(hd);
		}
		else {
			/* print what we got */
			hexdump_sent_print(hd, &b->buf[b->head], s);
		}

		/* update our head pointer */
		circular_buffer_advance_head(b, s);
	}

read_would_block:

	if (FD_ISSET(consumer, wfds)) {	 // ready to consume
		EINTR_RETRY(write(consumer, &b->buf[b->tail], circular_buffer_get_ready(b)));

		if (s < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				FD_CLR(consumer, wfds);
				goto write_would_block;
			}

			return -1;
		}
		else if (s == 0) {
			/* ack to the other end that we received the shutdown */
			partial_shutdown(ep_consumer, SHUT_WR);
			hexdump_shutdown_print(hd);
		}
		else {
			/* print how many is still here and we couldn't send */
			hexdump_remain_print(hd, s);
		}

		/* update our tail pointer */
		circular_buffer_advance_tail(b, s);

	}
	else if (FD_ISSET(producer, rfds)) {
		/* print how many is still here and we couldn't send */
		hexdump_remain_print(hd, 0);
	}

write_would_block:
	return 0;
}

/*
 * Set the file descriptors sets rfds and wfds for
 * reading/writing based on the available free space or
 * data ready in the buffer buf and based on if the producer
 * and or the consumer are not closed.
 *
 * Three values are possible:
 *  - 0 means that the pipe is still alive
 *  - 1 means that the producer is closed and no more data
 *	is ready to send (buffer is empty): shutdown the consumer
 *  - 2 means that the we have data ready to send but the consumer
 *	is closed so the pipe is broken: shutdown the producer
 *  */
enum pipe_status {
	PIPE_OPEN,
	PIPE_CLOSED,
	PIPE_BROKEN
};
enum pipe_status enable_read_write(struct endpoint *ep_producer,
		struct endpoint *ep_consumer,
		fd_set *rfds, fd_set *wfds,
		struct circular_buffer_t *buf) {

	int producer = ep_producer->fd;
	int consumer = ep_consumer->fd;

	/*
	 * Are our both endpoints, the consumer and the producer
	 * closed? If we have data in the pipe means that the pipe
	 * is broken, otherwise means that we are done, close the
	 * pipe
	 * */
	if (is_write_eof(ep_consumer) && is_read_eof(ep_producer)) {
		if (circular_buffer_get_ready(buf))
			return PIPE_BROKEN;
		else
			return PIPE_CLOSED;
	}

	/*
	 * If our consumer closed his side of the pipe means that we cannot
	 * write any data any more.
	 * If we still have data in the pipe that means that the
	 * pipe is broken, otherwise means that the pipe was closed
	 * by the consumer and this may be ok.
	 *
	 * In any case, a close by the consumer will imply a close
	 * for the producer as soon as possible.
	 *
	 * This may produce a broken pipe in the producer but we cannot
	 * know it because the producer didn't send us that data so from
	 * our point of view, everything is working normal.
	 * */
	if (is_write_eof(ep_consumer)) {
		partial_shutdown(ep_producer, SHUT_RD);

		if (circular_buffer_get_ready(buf))
			return PIPE_BROKEN;
		else
			return PIPE_CLOSED;
	}

	/*
	 * If the producer closed his side of the pipe means that we will
	 * not have more data in the pipe.
	 * If the pipe is already empty, close the consumer, closing the
	 * pipe, acknowling to the consumer that we are closing.
	 * If we still have data, keep the pipe alive as usual, we need to
	 * wait until the data in the pipe is flushed away to the consumer
	 * only then we need to close the pipe.
	 * */
	if (is_read_eof(ep_producer)) {
		if (!circular_buffer_get_ready(buf)) {
			partial_shutdown(ep_consumer, SHUT_WR);
			return PIPE_CLOSED;
		}
		else {
			/* keep the pipe and don't close the consumer
			 * we want to keep flushing all the data that
			 * we have in the pipe before closing it
			 * */
		}
	}

	/*
	 * If we have room in the buffer and the producer is not closed,
	 * enable it for reading, he may have more data for us.
	 * */
	if (circular_buffer_get_free(buf) && !is_read_eof(ep_producer))
		FD_SET(producer, rfds);

	/*
	 * If we have fresh data in the buffer to be sent to the consumer
	 * and the consumer is not closed, enable it for writing, he may
	 * want this data.
	 * */
	if (circular_buffer_get_ready(buf) && !is_write_eof(ep_consumer))
		FD_SET(consumer, wfds);

	return PIPE_OPEN;
}

int main(int argc, char *argv[]) {
	int ret = -1;
	int s;
	struct endpoint A, B;
	size_t buf_sizes[2] = {0, 0};
	size_t skt_buf_sizes[2] = {0, 0};
	char *out_filenames[2] = {0, 0};
	const char *colors[2] = {"\x1b[91m", "\x1b[94m"};
	int colorless = 0;
	sigset_t intset;

	if (parse_cmd_line(argc, argv, &A, &B, buf_sizes, skt_buf_sizes,
				out_filenames, &colorless)) {
		what(argv);
		usage(argv);
		return ret;
	}

	/*
	 * Block all the signals and setup some handlers for SIGINT and
	 * SIGTERM.
	 * Those are blocked but they will be unblock if the intset signal
	 * set mask is used.
	 *
	 * See block_all_signals and setup_signal_handlers to see exactly which
	 * signals are blocked and handled.
	 * */
	if (block_all_signals() != 0
			|| setup_signal_handlers() != 0
			|| initialize_interrupt_sigset(&intset) != 0) {
		perror("Setup signal handling failed");
		goto setup_signal_failed;
	}

	/* disable the colors? */
	if (colorless)
		colors[0] = colors[1] = 0;

	/* us <--> B */
	printf("Connecting to B %s:%s...\n", B.host, B.serv);
	if (establish_connection(&B, skt_buf_sizes, &intset) != 0) {
		perror("Establish a connection to the destination failed");
		goto establish_conn_failed;
	}

	/* A <--> us */
	printf("Waiting for a connection from A %s:%s...\n", A.host, A.serv);
	if (wait_for_connection(&A, skt_buf_sizes, &intset) != 0) {
		perror("Wait for connection from the source failed");
		goto wait_conn_failed;
	}

	printf("Allocating buffers: %lu and %lu bytes...\n", buf_sizes[0],
			buf_sizes[1]);
	struct circular_buffer_t buf_AtoB;
	if (circular_buffer_init(&buf_AtoB, buf_sizes[0]) != 0) {
		perror("Buffer allocation for A->B failed");
		goto buf_AtoB_failed;
	}

	struct circular_buffer_t buf_BtoA;
	if (circular_buffer_init(&buf_BtoA, buf_sizes[1]) != 0) {
		perror("Buffer allocation for B->A failed");
		goto buf_BtoA_failed;
	}

	struct hexdump hd_AtoB;
	if (hexdump_init(&hd_AtoB, "A", "B", colors[0], out_filenames[0]) != 0) {
		perror("Hexdump A->B allocation failed");
		goto hd_A_to_B_failed;
	}

	struct hexdump hd_BtoA;
	if (hexdump_init(&hd_BtoA, "B", "A", colors[1], out_filenames[1]) != 0) {
		perror("Hexdump B->A allocation failed");
		goto hd_B_to_A_failed;
	}

	int nfds = MAX(A.fd, B.fd) + 1;
	fd_set rfds, wfds;

	enum pipe_status pstatus_AtoB = PIPE_OPEN;
	enum pipe_status pstatus_BtoA = PIPE_OPEN;

	while (1) {
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		if (pstatus_AtoB == PIPE_OPEN)
			pstatus_AtoB = enable_read_write(&A, &B,
					&rfds, &wfds,
					&buf_AtoB);

		if (pstatus_BtoA == PIPE_OPEN)
			pstatus_BtoA = enable_read_write(&B, &A,
					&rfds, &wfds,
					&buf_BtoA);


		if (pstatus_AtoB != PIPE_OPEN && pstatus_BtoA != PIPE_OPEN)
			break; /* we finished: no data can be sent from
				  A to B nor B to A. */


		EINTR_RETRY(pselect(nfds, &rfds, &wfds, NULL, NULL, &intset));

		if (s == -1) {
			perror("select call failed");
			goto passthrough_failed;
		}

		if (passthrough(&A, &B, &rfds, &wfds, &buf_AtoB, &hd_AtoB) != 0) {
			perror("Passthrough from A to B failed");
			goto passthrough_failed;
		}

		if (passthrough(&B, &A, &rfds, &wfds, &buf_BtoA, &hd_BtoA) != 0) {
			perror("Passthrough from B to A failed");
			goto passthrough_failed;
		}
	}

	ret = 0;

passthrough_failed:
	hexdump_destroy(&hd_BtoA);

hd_B_to_A_failed:
	hexdump_destroy(&hd_AtoB);

hd_A_to_B_failed:
	circular_buffer_destroy(&buf_BtoA);

buf_BtoA_failed:
	circular_buffer_destroy(&buf_AtoB);

buf_AtoB_failed:
	shutdown_and_close(&A);

wait_conn_failed:
	shutdown_and_close(&B);

establish_conn_failed:
setup_signal_failed:

	if (!colorless)
		printf("%s", "\x1b[0m"); /* reset */
	if (interrupted)
		printf("\nUser cancelled.\n");

	if (out_filenames[0]) {
		free(out_filenames[0]);
	}
	if (out_filenames[1]) {
		free(out_filenames[1]);
	}

	return interrupted?  128 + interrupted : ret;
}
