#ifndef CMDLINE_H_
#define CMDLINE_H_

struct endpoint;

int parse_cmd_line(int argc, char *argv[], struct endpoint *A,
		struct endpoint *B, size_t buf_sizes[2],
		size_t skt_buf_sizes[2], char *out_filenames[2],
		int *colorless);

void what(char *argv[]);
void usage(char *argv[]);


#endif
