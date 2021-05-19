#define _POSIX_C_SOURCE 200112L

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
#define TIBURONCIN_VERSION "2.2.0"

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

static
int parse_output_filenames(char *prefix, char *out_filenames[]) {
	int prefix_len = strlen(prefix);

	/* the size of a literal string is the length of the string plus one, because of the '\0' */
	int atob_size = prefix_len + sizeof(DEFAULT_A_TO_B_DUMPFILENAME);
	int btoa_size = prefix_len + sizeof(DEFAULT_B_TO_A_DUMPFILENAME);

	out_filenames[0] = malloc(atob_size);
	out_filenames[1] = malloc(btoa_size);

	/* snprintf receives the size of the destination buffer (including the '\0' slot), but it
	 * returns the lenght of the resulting string (excluding the '\0' slot) */
	if (snprintf(out_filenames[0], atob_size, "%s%s", prefix, DEFAULT_A_TO_B_DUMPFILENAME) != atob_size - 1)
		return -1;

	if (snprintf(out_filenames[1], btoa_size, "%s%s", prefix, DEFAULT_B_TO_A_DUMPFILENAME) != btoa_size - 1)
		return -1;

	return 0;
}

static
int save_default_output_filenames(char *out_filenames[]) {
	out_filenames[0] = malloc(sizeof(DEFAULT_A_TO_B_DUMPFILENAME));
	if (out_filenames[0] == NULL) {
		return -1;
	}

	out_filenames[1] = malloc(sizeof(DEFAULT_B_TO_A_DUMPFILENAME));
	if (out_filenames[1] == NULL) {
		free(out_filenames[0]);
		return -1;
	}

	/* memcpy doesn't fail */
	memcpy(out_filenames[0], DEFAULT_A_TO_B_DUMPFILENAME, sizeof(DEFAULT_A_TO_B_DUMPFILENAME));
	memcpy(out_filenames[1], DEFAULT_B_TO_A_DUMPFILENAME, sizeof(DEFAULT_B_TO_A_DUMPFILENAME));
	return 0;
}

int parse_cmd_line(int argc, char *argv[], struct endpoint *A,
		struct endpoint *B, size_t buf_sizes[2],
		size_t skt_buf_sizes[2], char *out_filenames[2],
		int *colorless) {
	int ret = -1;
	int opt;
	int opt_found = 0;

	/* default values */
	buf_sizes[0] = buf_sizes[1] = DEFAULT_BUF_SIZE;
	skt_buf_sizes[0] = skt_buf_sizes[1] = 0;
	out_filenames[0] = out_filenames[1] = 0;
	*colorless = 0;

	while ((opt = getopt(argc, argv, "A:B:b:z:ochf:")) != -1) {
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
				opt_found |= 8;
				if (opt_found & 4) {
					fprintf(stderr, "Options -o and -f are incompatible.\n");
					return ret;
				}
				if (save_default_output_filenames(out_filenames)) {
					fprintf(stderr, "Error while saving default output filenames.\n");
					return ret;
				}
				break;

			case 'f':
				/* add output files prefix */
				opt_found |= 4;
				if (opt_found & 8) {
					fprintf(stderr, "Options -o and -f are incompatible.\n");
					return ret;
				}
				if (parse_output_filenames(optarg, out_filenames) != 0) {
					fprintf(stderr, "Invalid output filenames prefix.\n");
					return ret;
				}
				break;

			case 'c':
				/* color less */
				*colorless = 1;
				break;

			case 'h':
				return ret;

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
	printf
		("tiburoncin\n"
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
	printf
		("%s -A <addr> -B <addr> [-b <bsz>] [-z <bsz>] [-o | -f <prefix>] [-c]\n"
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
		 " running 'xxd -p -c 16 -r <raw hexdump file>'. See man xxd(1)\n"
		 " This option is incompatible with -f option\n"
		 " \n"
		 " -f <prefix> same as -o, but it pre-concatenates the specified prefix\n"
		 " This option is incompatible with -o option\n"
		 " \n"
		 " -c disable the color in the output (colorless)\n",
		argv[0], DEFAULT_HOST, DEFAULT_BUF_SIZE,
		DEFAULT_A_TO_B_DUMPFILENAME, DEFAULT_B_TO_A_DUMPFILENAME);
}

#undef _POSIX_C_SOURCE

