#ifndef CMDLINE_H_
#define CMDLINE_H_

struct endpoint;

int parse_cmd_line(int argc, char *argv[], struct endpoint *src,
		struct endpoint *dst, size_t buf_sizes[2],
		size_t skt_buf_sizes[2], const char *out_filenames[2]);

void usage(char *argv[]);


#endif
