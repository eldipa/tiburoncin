#ifndef SOCKET_H_
#define SOCKET_H_

#define FLOW_RD 1
#define FLOW_WR 2
#define FLOW_RDWR (FLOW_RD|FLOW_WR)

struct endpoint;

/*
 * Wait for a connecion on host:serv given in the endpoint src.
 *
 * Save the file descriptor of the peer socket if it succeeds into src
 * and return 0.
 * On error, return -1 and errno is set appropriately.
 * */
int wait_for_connection(struct endpoint *src, size_t skt_buf_sizes[2]);

/* 
 * Establish a connection to host:serv defined in the endpoint dst.
 *
 * Save the file descriptor of the peer socket if it succeeds into dst
 * and return 0.
 * On error, return -1 and errno is set appropriately.
 * */
int establish_connection(struct endpoint *dst, size_t skt_buf_sizes[2]);


/*
 * Shutdown any open flow and close the endpoint 
 * */
void shutdown_and_close(struct endpoint *p);


void partial_shutdown(struct endpoint *p, int direction);
int is_read_eof(struct endpoint *p);
int is_write_eof(struct endpoint *p);

#endif
