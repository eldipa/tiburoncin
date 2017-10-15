#define _POSIX_C_SOURCE 9999999

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "endpoint.h"
#include "socket.h"

#define DEFAULT_BACKLOG 1


/*
 * Given a hostname and servicename in the endpoint p,
 * return the result of the lookup into *result.
 *
 * Return 0 if succeeds. On error, return -1, set
 * the errno appropriately and print to stderr a
 * descriptive message.
 *
 * See man page of getaddrinfo and gai_strerror.
 * */
static
int resolv(struct endpoint *p, struct addrinfo **result) {
	struct addrinfo hints;
	int s;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((s = getaddrinfo(p->host, p->serv, &hints, result)) != 0) {
		fprintf(stderr, "Address resolution failed for %s:%s: %s\n",
				p->host, p->serv, gai_strerror(s));

		errno = EADDRNOTAVAIL; /* quite arbitrary as it is not easy
					  to map a gai error into a errno */

		return -1;
	}
	else {
		return 0;
	}
}

static
int set_socket_buffer_sizes(int fd, size_t skt_buf_sizes[2]) {
	int optnames[2] = { SO_SNDBUF, SO_RCVBUF };
	for (int i = 0; i < 2; ++i) {
		if (!skt_buf_sizes[i])
			continue;

		int val = (int)skt_buf_sizes[i];
		if (val <= 0) {
			errno = ERANGE;
			return -1;
		}

		int opt = optnames[i];
		if (setsockopt(fd, SOL_SOCKET, opt, &val, sizeof(val)) == -1)
			return -1;
	}

	return 0;
}

/*
 * Create a socket in listening mode for accepting connections.
 * The socket will be listening on host:serv address set by src.
 *
 * Return a valid file descriptor if it succeeds,
 * -1 if not.
 *  In case of error, errno is set appropriately.
 *  */
static
int set_listening(struct endpoint *src, size_t skt_buf_sizes[2]) {
	int ret = -1;
	int val = 1;

	int fd = -1;
	int s;
	int last_errno = 0;
	struct addrinfo *result, *rp;

	s = resolv(src, &result);
	if (s != 0) 
		goto resolv_failed;

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		
		if (fd == -1) {
			last_errno = errno;
			continue;
		}

		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) == -1) {
			last_errno = errno;
			continue;
		}
		
		if (set_socket_buffer_sizes(fd, skt_buf_sizes) != 0) {
			last_errno = errno;
			continue;
		}

		if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0)
			if (listen(fd, DEFAULT_BACKLOG) == 0)
				break;	/* good */

		/* bad */
		last_errno = errno;
		close(fd);	
	}
	
	freeaddrinfo(result);
	errno = last_errno;

	if (rp != NULL) {
		src->fd = fd;
		ret = 0;
	}


resolv_failed:
	return ret;
}

/*
 * Wait for a connecion on host:serv given in the endpoint src.
 *
 * Save the file descriptor of the peer socket if it succeeds into src
 * and return 0.
 * On error, return -1 and errno is set appropriately.
 * */
int wait_for_connection(struct endpoint *src, size_t skt_buf_sizes[2]) {
	int ret = -1;

	if (set_listening(src, skt_buf_sizes) == -1)
		goto listening_failed;

	int passive_fd = src->fd;
	int peerfd = accept(passive_fd, NULL, NULL);
	if (peerfd > 0) {
		src->fd = peerfd;
		src->eof = 0;
		ret = 0;
	}

	shutdown(passive_fd, SHUT_RDWR);
	close(passive_fd);
	
listening_failed:
	return ret;
}

/* 
 * Establish a connection to host:serv defined in the endpoint dst.
 *
 * Save the file descriptor of the peer socket if it succeeds into dst
 * and return 0.
 * On error, return -1 and errno is set appropriately.
 * */
int establish_connection(struct endpoint *dst, size_t skt_buf_sizes[2]) {
	int ret = -1;
	
	int fd;
	int s;
	int last_errno = 0;
	struct addrinfo *result, *rp;

	s = resolv(dst, &result);
	if (s != 0) 
		goto resolv_failed;

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (fd == -1) {
			last_errno = errno;
			continue;
		}
		
		if (set_socket_buffer_sizes(fd, skt_buf_sizes) != 0) {
			last_errno = errno;
			continue;
		}

		if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;	/* good */

		/* bad */
		last_errno = errno;
		close(fd);	
	}
	
	freeaddrinfo(result);
	errno = last_errno;

	if (rp != NULL) {
		dst->fd = fd;
		dst->eof = 0;
		ret = 0;
	}

resolv_failed:
	return ret;

}

void partial_shutdown(struct endpoint *p, int direction) {
	shutdown(p->fd, direction);
	p->eof |= (direction+1);
}

int is_read_eof(struct endpoint *p) {
	return p->eof & 1;
}

int is_write_eof(struct endpoint *p) {
	return p->eof & 2;
}

#include <syslog.h>
void shutdown_and_close(struct endpoint *p) {
	if (!is_read_eof(p))
		partial_shutdown(p, SHUT_RD);

	if (!is_write_eof(p))
		partial_shutdown(p, SHUT_WR);

	close(p->fd);
}


#undef _POSIX_C_SOURCE
