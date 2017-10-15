#ifndef ENDPOINT_H_
#define ENDPOINT_H_

struct endpoint {
	char *host;
	char *serv;

	int fd;
	int eof;
};

#endif
