#ifndef SOCKET_H_
#define SOCKET_H_

#define FLOW_RD 1
#define FLOW_WR 2
#define FLOW_RDWR (FLOW_RD|FLOW_WR)

#include "signal.h"


/* struct endpoint: a wrapper around nonblocking sockets.
 *
 * To establish a TCP channel you have to call wait_for_connection
 * in one endpoint and establish_connection in the other.
 *
 * The former creates a *temporal* TCP socket for accepting which
 * will be close before returning.
 * This function returns a new nonblocking file descriptor
 * representing the other endpoint (see wait_for_connection).
 *
 * The latter creates a nonblocking file descriptor and connects
 * to the other endpoint (see establish_connection)
 *
 * Both are blocking functions despite that they use nonblocking
 * sockets.
 * */
struct endpoint;

/*
 * Wait for a connection on host:serv given in the endpoint A.
 * During the wait, set the signal mask set atomically before blocking.
 *
 * Save the file descriptor of the peer socket if it succeeds into A
 * and return 0.
 * On error, return -1 and errno is set appropriately.
 * */
int wait_for_connection(struct endpoint *A, size_t skt_buf_sizes[2],
		sigset_t *set);

/*
 * Establish a connection to host:serv defined in the endpoint B.
 * During the connection, set the signal mask set atomically before blocking.
 *
 * Save the file descriptor of the peer socket if it succeeds into B
 * and return 0.
 * On error, return -1 and errno is set appropriately.
 *
 * Note: the function will try to connect to the endpoint several times
 * (CONNECT_TRIES == 3 by default), sleeping for CONNECT_WAIT_TRY_SECS == 1
 * seconds between each try, only then it will give up.
 *
 * The signal mask *will not be changed* during this wait.
 * */
int establish_connection(struct endpoint *B, size_t skt_buf_sizes[2],
		sigset_t *set);


/*
 * Shutdown any open flow and close the endpoint
 * */
void shutdown_and_close(struct endpoint *p);


void partial_shutdown(struct endpoint *p, int direction);
int is_read_eof(struct endpoint *p);
int is_write_eof(struct endpoint *p);

#endif
