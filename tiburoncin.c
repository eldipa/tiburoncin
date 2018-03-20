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

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int passthrough(struct endpoint *ep_producer, struct endpoint *ep_consumer,
		fd_set *rfds, fd_set *wfds,
		struct circular_buffer_t *b,
		struct hexdump *hd) {
	int producer = ep_producer->fd;
	int consumer = ep_consumer->fd;
	int s;

	if (FD_ISSET(producer, rfds)) {	 // ready to produce
		s = read(producer, &b->buf[b->head], circular_buffer_get_free(b));
		if (s < 0)
			return -1;

		/* ack to the other end that we received the shutdown */
		if (s == 0)
			partial_shutdown(ep_producer, SHUT_RD);

		/* print what we got */
		hexdump_sent_print(hd, &b->buf[b->head], s);

		/* update our head pointer */
		circular_buffer_advance_head(b, s);
	}

	if (FD_ISSET(consumer, wfds)) {	 // ready to consume
		s = write(consumer, &b->buf[b->tail], circular_buffer_get_ready(b));
		if (s < 0)
			return -1;

		/* ack to the other end that we received the shutdown */
		if (s == 0)
			partial_shutdown(ep_consumer, SHUT_WR);

		/* print what we don't got */
		hexdump_remain_print(hd, s);

		/* update our tail pointer */
		circular_buffer_advance_tail(b, s);

	}
	else if (FD_ISSET(producer, rfds)) {
		/* print what we don't got */
		hexdump_remain_print(hd, 0);
	}

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
	struct endpoint src, dst;
	size_t buf_sizes[2] = {0, 0};
	size_t skt_buf_sizes[2] = {0, 0};
	const char *out_filenames[2] = {0, 0};

	if (parse_cmd_line(argc, argv, &src, &dst, buf_sizes, skt_buf_sizes,
				out_filenames)) {
		usage(argv);
		return ret;
	}

	/* us <--> dst */
	printf("Connecting to dst %s:%s...\n", dst.host, dst.serv);
	if (establish_connection(&dst, skt_buf_sizes) != 0) {
		perror("Establish a connection to the destination failed");
		goto establish_conn_failed;
	}

	/* src <--> us */
	printf("Waiting for a connection from src %s:%s...\n", src.host, src.serv);
	if (wait_for_connection(&src, skt_buf_sizes) != 0) {
		perror("Wait for connection from the source failed");
		goto wait_conn_failed;
	}

	printf("Allocating buffers: %lu and %lu bytes...\n", buf_sizes[0],
					 			buf_sizes[1]);
	struct circular_buffer_t buf_stod;
	if (circular_buffer_init(&buf_stod, buf_sizes[0]) != 0) {
		perror("Buffer allocation for src->dst failed");
		goto buf_stod_failed;
	}

	struct circular_buffer_t buf_dtos;
	if (circular_buffer_init(&buf_dtos, buf_sizes[1]) != 0) {
		perror("Buffer allocation for dst->src failed");
		goto buf_dtos_failed;
	}

	struct hexdump hd_stod;
	if (hexdump_init(&hd_stod, "src", "dst", "\x1b[91m", out_filenames[0]) != 0) {
		perror("Hexdump src->dst allocation failed");
		goto hd_src_to_dst_failed;
	}

	struct hexdump hd_dtos;
	if (hexdump_init(&hd_dtos, "dst", "src", "\x1b[94m", out_filenames[1]) != 0) {
		perror("Hexdump dst->src allocation failed");
		goto hd_dst_to_src_failed;
	}

	int nfds = MAX(src.fd, dst.fd) + 1;
	fd_set rfds, wfds;

	enum pipe_status pstatus_stod = PIPE_OPEN;
	enum pipe_status pstatus_dtos = PIPE_OPEN;

	while (1) {
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);

		if (pstatus_stod == PIPE_OPEN)
			pstatus_stod = enable_read_write(&src, &dst,
						&rfds, &wfds,
						&buf_stod);

		if (pstatus_dtos == PIPE_OPEN)
			pstatus_dtos = enable_read_write(&dst, &src,
						&rfds, &wfds,
						&buf_dtos);


		if (pstatus_stod != PIPE_OPEN && pstatus_dtos != PIPE_OPEN)
			break; /* we finished: no data can be sent from
				  src to dst nor dst to src. */


		s = select(nfds, &rfds, &wfds, NULL, NULL);
		if (s == -1) {
			perror("select call failed");
			goto passthrough_failed;
		}

		if (passthrough(&src, &dst, &rfds, &wfds, &buf_stod, &hd_stod) != 0) {
			perror("Passthrough from src to dst failed");
			goto passthrough_failed;
		}

		if (passthrough(&dst, &src, &rfds, &wfds, &buf_dtos, &hd_dtos) != 0) {
			perror("Passthrough from dst to src failed");
			goto passthrough_failed;
		}
	}

	ret = 0;

passthrough_failed:
	hexdump_destroy(&hd_dtos);

hd_dst_to_src_failed:
	hexdump_destroy(&hd_stod);

hd_src_to_dst_failed:
	circular_buffer_destroy(&buf_dtos);

buf_dtos_failed:
	circular_buffer_destroy(&buf_stod);

buf_stod_failed:
	shutdown_and_close(&src);

wait_conn_failed:
	shutdown_and_close(&dst);

establish_conn_failed:
	return ret;
}
