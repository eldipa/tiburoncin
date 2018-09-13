#ifndef HEXDUMP_H_
#define HEXDUMP_H_

#include <stdio.h>

struct hexdump {
	unsigned int offset_consumer;
	unsigned int offset;
	const char *from;
	const char *to;

	const char *color_escape;
	FILE *out_file;
};

int hexdump_init(struct hexdump *hd, const char *from, const char *to,
		const char *color_escape, const char out_filename[64]);
void hexdump_sent_print(struct hexdump *hd, const char *buf, unsigned int sz);
void hexdump_remain_print(struct hexdump *hd, unsigned int sz);
void hexdump_shutdown_print(struct hexdump *hd);
void hexdump_destroy(struct hexdump *hd);

#endif
