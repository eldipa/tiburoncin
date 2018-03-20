
#define _POSIX_C_SOURCE 9999999

#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>

#include "endpoint.h"
#include "cmdline.h"

#define DEFAULT_HOST "localhost"
#define DEFAULT_BUF_SIZE (2048)

#define DEFAULT_A_TO_B_DUMPFILENAME "AtoB.dump"
#define DEFAULT_B_TO_A_DUMPFILENAME "BtoA.dump"

#define TIBURONCIN_AUTHOR "Martin Di Paola"
#define TIBURONCIN_URL "https://github.com/eldipa/tiburoncin"
#define TIBURONCIN_LICENSE "GPLv3"
#define TIBURONCIN_VERSION "1.0.0"

extern char *optarg;

static
void parse_address(char *addr_str, char **host, char **serv) {
	char *colon = strrchr(addr_str, ':');

	if (colon) {
		*colon = 0;

		if (addr_str[0]) {
			/* form host:service */
			*host = addr_str;
			*serv = colon + 1;
		}
		else {
			/* form :service  (localhost) */
			*host = DEFAULT_HOST;
			*serv = colon + 1;
		}
	}
	else {
		/* form service  (localhost) */
		*host = DEFAULT_HOST;
		*serv = addr_str;
	}
}

static
int parse_buffer_sizes(char *sz_str, size_t buf_sizes[2]) {
	char *colon = strrchr(sz_str, ':');

	long long int values[2] = {0, 0};

	if (colon) {
		*colon = 0;
		values[0] = strtoll(sz_str, NULL, 0);
		values[1] = strtoll(colon+1, NULL, 0);
	}
	else {
		values[0] = values[1] = strtoll(sz_str, NULL, 0);

	}

	for (int i = 0; i < 2; ++i) {
		if (values[i] == LLONG_MIN || values[i] == LLONG_MAX)
			return -1;

		if (values[i] <= 0 || values[i] > SIZE_MAX) {
			errno = ERANGE;
			return -1;
		}

		buf_sizes[i] = (size_t) values[i];
	}

	return 0;
}

int parse_cmd_line(int argc, char *argv[], struct endpoint *A,
		struct endpoint *B, size_t buf_sizes[2],
		size_t skt_buf_sizes[2], const char *out_filenames[2]) {
	int ret = -1;
	int opt;
	int opt_found = 0;

	/* default values */
	buf_sizes[0] = buf_sizes[1] = DEFAULT_BUF_SIZE;
	skt_buf_sizes[0] = skt_buf_sizes[1] = 0;
	out_filenames[0] = out_filenames[1] = 0;

	while ((opt = getopt(argc, argv, "A:B:b:z:o")) != -1) {
		switch (opt) {
			case 'A':
				/* A configuration */
				parse_address(optarg, &A->host, &A->serv);
				opt_found |= 1;
				break;

			case 'B':
				/* B configuration */
				parse_address(optarg, &B->host, &B->serv);
				opt_found |= 2;
				break;

			case 'b':
				/* buffer sizes configuration */
				if (parse_buffer_sizes(optarg, buf_sizes) != 0) {
					fprintf(stderr, "Invalid buffer size.\n");
					return ret;
				}
				break;

			case 'z':
				/* socket's buffer sizes configuration */
				if (parse_buffer_sizes(optarg, skt_buf_sizes) != 0) {
					fprintf(stderr, "Invalid socket buffer size.\n");
					return ret;
				}
				break;

			case 'o':
				/* save capture onto the output files */
				out_filenames[0] = DEFAULT_A_TO_B_DUMPFILENAME;
				out_filenames[1] = DEFAULT_B_TO_A_DUMPFILENAME;
				break;

			default:
				fprintf(stderr, "Unknown argument.\n");
				return ret;

		}
	}

	if ((opt_found & (1 | 2)) != (1 | 2)) {
		fprintf(stderr, "Missing arguments. You need to pass -A and -B flags.\n");
		return ret;
	}

	ret = 0;
	return ret;
}

void what(char *argv[]) {
	printf("tiburoncin\n"
	       "==========\n"
	       "\n"
	       "Small man in the middle tool to inspect the traffic between two endpoints A and B.\n"
	       "Mostly to be used for students wanting to know what data is flowing in a TCP channel.\n"
	       "\n"
	       "Author: %s\n"
	       "URL: %s\n"
	       "License: %s\n"
	       "Version: %s\n"
	       "\n",
		TIBURONCIN_AUTHOR, TIBURONCIN_URL,
		TIBURONCIN_LICENSE, TIBURONCIN_VERSION);
}

void usage(char *argv[]) {
	printf("%s -A <addr> -B <addr> [-b <bsz>] [-z <bsz>] [-o]\n"
	       " where <addr> can be of the form:\n"
	       "  - host:serv\n"
	       "  - :serv\n"
	       "  - serv\n"
	       " If it is not specified, host will be %s.\n"
	       " In all the cases the host can be a hostname or an IP;\n"
	       " for the service it can be a servicename or a port number.\n"
	       " \n"
	       " -b <bsz> sets the buffer size of tiburocin\n"
	       " where <bsz> is a size in bytes of the form:\n"
	       "  - num      sets the size of both buffers to that value\n"
	       "  - num:num  sets sizes for A->B and B->A buffers\n"
	       " by default, both buffers are of %i bytes\n"
	       " \n"
	       " -z <bsz> sets the buffer size of the sockets\n"
	       " where <bsz> is a size in bytes of the form:\n"
	       "  - num      sets the size of both buffers SND and RCV to that value\n"
	       "  - num:num  sets sizes for SND and RCV buffers\n"
	       " by default, both buffers are not changed. See man socket(7)\n"
	       " \n"
	       " -o save the received data onto two files:\n"
	       "  %s for the data received from A\n"
	       "  %s for the data received from B\n"
	       " in both cases a raw hexdump is saved which can be recovered later\n"
	       " running 'xxd -p -c 16 -r <raw hexdump file>'. See man xxd(1)\n",
	       argv[0], DEFAULT_HOST, DEFAULT_BUF_SIZE,
			DEFAULT_A_TO_B_DUMPFILENAME, DEFAULT_B_TO_A_DUMPFILENAME);
}

#undef _POSIX_C_SOURCE

