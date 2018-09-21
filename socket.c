#define _POSIX_C_SOURCE 200112L

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include <sys/select.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "endpoint.h"
#include "socket.h"
#include "signal.h"

#define DEFAULT_BACKLOG 1
#define CONNECT_TRIES 3
#define CONNECT_WAIT_TRY_SECS 1

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

		if (s != EAI_SYSTEM)
			errno = EADDRNOTAVAIL; /* quite arbitrary as it is not easy
						  to map a gai error into a errno */

		return -1;
	}
	else {
		return 0;
	}
}

/*
 * Set the socket as non blocking.
 * On error, return -1 and errno is set appropriately; 0 otherwise.
 * */
static
int set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		return -1;

	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
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
 * The socket will be listening on host:serv address set by A.
 *
 * Return a valid file descriptor if it succeeds,
 * -1 if not.
 *  In case of error, errno is set appropriately.
 *  */
static
int set_listening(struct endpoint *A, size_t skt_buf_sizes[2]) {
	int ret = -1;
	int val = 1;

	int fd = -1;
	int s;
	int last_errno = 0;
	struct addrinfo *result, *rp;

	s = resolv(A, &result);
	if (s != 0)
		goto resolv_failed;

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (fd == -1) {
			last_errno = errno;
			continue;
		}

		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != -1
				&& set_socket_buffer_sizes(fd, skt_buf_sizes) != -1
				&& set_nonblocking(fd) != -1
				&& bind(fd, rp->ai_addr, rp->ai_addrlen) != -1
				&& listen(fd, DEFAULT_BACKLOG) != -1) {
			break;	/* good */
		}
		else {
			/* bad */
			last_errno = errno;
			EINTR_RETRY(close(fd));
		}
	}

	freeaddrinfo(result);
	errno = last_errno;

	if (rp != NULL) {
		A->fd = fd;
		ret = 0;
	}

resolv_failed:
	return ret;
}

/*
 * Connect to socket fd to the destination described in rp.
 * This virtually works like the connect syscall (see connect(2)) but allows
 * to set a signal mask before blocking atomically.
 *
 * Note: the file descriptor must have the O_NONBLOCK flag set
 * (non blocking mode, see fcntl(2))
 *
 * Return -1 on error, (errno is set appropriately) 0 on success.
 *
 * Note: this function may fail under different circumstances which may
 * produce the same error number (errno) due different causes:
 *  - the connect syscall could fail
 *  - the pselect syscall could fail
 *  - the getsockopt could fail (used to get the status of the connection)
 *  - the connection could fail
 **/
static
int pconnect(int fd, struct addrinfo *rp, sigset_t *set) {
	int s;
	EINTR_RETRY(connect(fd, rp->ai_addr, rp->ai_addrlen));

	/*
	 * The connection failed, and failed quickly. We should fail too.
	 * */
	if (s == -1 && errno != EINPROGRESS)
		return -1;

	/*
	 * The connection was established quickly, we are done.
	 * */
	if (s != -1)
		return 0;

	/*
	 * The connection didn't fail but it is still in progress,
	 * so wait for it.
	 * See connect(2)
	 * */
	fd_set wfds;
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);

	EINTR_RETRY(pselect(fd + 1, NULL, &wfds, NULL, NULL, set));

	/*
	 * The wait failed, we should fail too.
	 * */
	if (s == -1)
		return -1;

	/*
	 * See if the connection succeed or not.
	 * See socket(7)
	 * */
	int val = -1;
	size_t vlen = sizeof(val);
	s = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&val, (void*)&vlen);

	/*
	 * We couldn't retrieved the socket error (if any), fail.
	 * */
	if (s == -1)
		return -1;

	if (val != 0) {
		errno = val; /* TODO Is it possible that val == EAGAIN/EWOULDBLOCK/EINPROGRESS like in the accept? */
		return -1;
	}

	/*
	 * The connection was established, hurra!
	 * */
	return 0;
}

/*
 * Accept a new connection to passive_fd.
 * This virtually works like the accept syscall (see accept(2)) but
 * in addition allows to set the signal mask set before blocking atomically.
 *
 * Note: the file descriptor must have the O_NONBLOCK flag set
 * (non blocking mode, see fcntl(2))
 *
 * If the function succeeds, the returned file descriptor will also have
 * the O_NONBLOCK flag.
 *
 * Return -1 on error, (errno is set appropriately), otherwise return
 * a new file descriptor that represents the other endpoint of the
 * connection.
 *
 * Note: this function may fail under different circumstances which may
 * produce the same error number (errno) due different causes:
 *  - the pselect syscall could fail
 *  - the accept syscall could fail
 * */
static
int paccept(int passive_fd, sigset_t *set) {
	int s = -1;

	fd_set rfds;
	do {
		FD_ZERO(&rfds);
		FD_SET(passive_fd, &rfds);
		EINTR_RETRY(pselect(passive_fd + 1, &rfds, NULL, NULL, NULL, set));

		/* select failed, we cannot do much more, fail too. */
		if (s == -1)
			return -1;

		EINTR_RETRY(accept(passive_fd, NULL, NULL));

		/* accept failed and it is not due a false positive read event
		 * (in those cases, the accept syscall "would block")
		 * Fail directly.
		 * */
		if (s == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
			return -1;

	} while(s == -1);

	return s; // new file descriptor
}

/*
 * Wait for a connection on host:serv given in the endpoint A.
 * During the wait, set the signal mask set atomically before blocking.
 *
 * Save the file descriptor of the peer socket if it succeeds into A
 * and return 0.
 * On error, return -1 and errno is set appropriately.
 * */
int wait_for_connection(struct endpoint *A, size_t skt_buf_sizes[2],
		sigset_t *set) {
	int ret = -1;
	int s = -1;
	int last_errno = 0;

	if (set_listening(A, skt_buf_sizes) == -1) {
		last_errno = errno;
		goto listening_failed;
	}

	int passive_fd = A->fd;
	s = paccept(passive_fd, set);
	if (s == -1) {
		last_errno = errno;
		goto accept_failed;
	}

	int fd = s;
	s = set_nonblocking(fd);
	if (s == -1) {
		last_errno = errno;

		shutdown(fd, SHUT_RDWR);
		EINTR_RETRY(close(fd));	// TODO error is ignored
		goto accept_failed;
	}

	A->fd = fd;
	A->eof = 0;
	ret = 0;

accept_failed:
	shutdown(passive_fd, SHUT_RDWR);
	EINTR_RETRY(close(passive_fd));	// TODO error is ignored

listening_failed:
	errno = last_errno;
	return ret;
}

int establish_connection(struct endpoint *B, size_t skt_buf_sizes[2],
		sigset_t *set) {
	int ret = -1;

	int fd;
	int s;
	int last_errno = 0;
	struct addrinfo *result, *rp;
	int remain = CONNECT_TRIES;

	do {
		s = resolv(B, &result);
		if (s != 0) {
			last_errno = errno;
			result = NULL;
		}

		for (rp = result; rp != NULL; rp = rp->ai_next) {
			fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

			if (fd == -1) {
				last_errno = errno;
				continue;
			}

			if (set_socket_buffer_sizes(fd, skt_buf_sizes) != -1
					&& set_nonblocking(fd) != -1
					&& pconnect(fd, rp, set) != -1 ) {
				break;	/* good */
			}
			else {
				/* bad */
				last_errno = errno;
				EINTR_RETRY(close(fd));
			}
		}

		if (result != NULL)
			freeaddrinfo(result);

		if (rp == NULL && --remain > 0) {
			struct timespec t = {
				.tv_sec  = CONNECT_WAIT_TRY_SECS,
				.tv_nsec = 0
			};
			EINTR_RETRY(nanosleep(&t, &t)); /* ignore any error */
		}

	} while (rp == NULL && remain > 0);

	if (rp != NULL) {
		B->fd = fd;
		B->eof = 0;
		ret = 0;
	}

	errno = last_errno;
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

void shutdown_and_close(struct endpoint *p) {
	int s;
	if (!is_read_eof(p))
		partial_shutdown(p, SHUT_RD);

	if (!is_write_eof(p))
		partial_shutdown(p, SHUT_WR);

	EINTR_RETRY(close(p->fd));
}

