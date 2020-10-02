#include "hexdump.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int hexdump_init(struct hexdump *hd, const char *from, const char *to,
		const char *color_escape, const char *out_filename) {
	memset(hd, 0, sizeof(*hd));
	hd->from = from;
	hd->to = to;
	hd->color_escape = color_escape;

	if (!out_filename)
		return 0;

	hd->out_file = fopen(out_filename, "wt");
	if (!hd->out_file)
		return -1;

	return 0;
}

void hexdump_destroy(struct hexdump *hd) {
	if (hd->out_file)
		fclose(hd->out_file);
}

size_t print_half_hex(struct hexdump *hd, size_t begin, int half,
		const char *buf, unsigned int sz) {

	size_t offset = half==0? begin : begin + 8;
	size_t end = offset + 8;
	size_t i = 0;

	for (; offset < end && i < sz; ++offset)  {
		if (offset < hd->offset) {
			printf("   ");
		}
		else {
			printf("%02hhx ", buf[i]);
			++i;

		}
	}

	for (; offset < end; ++offset)
		printf("   ");

	return i;
}

size_t print_ascii(struct hexdump *hd, size_t begin,
		const char *buf, unsigned int sz) {

	size_t offset = begin;
	size_t end = begin + 16;
	size_t i = 0;

	for (; offset < end && i < sz; ++offset)  {
		if (offset < hd->offset) {
			printf(" ");
		}
		else {
			char c = isprint(buf[i])? buf[i] : '.';
			printf("%c", c);
			++i;
		}
	}

	for (; offset < end; ++offset)
		printf(" ");

	return i;
}

size_t print_full_hex(struct hexdump *hd, size_t start_line_offset,
		const char *buf, unsigned int sz) {
	size_t consumed = print_half_hex(hd, start_line_offset, 0, buf, sz);

	buf += consumed;
	sz  -= consumed;

	printf(" ");
	consumed += print_half_hex(hd, start_line_offset, 1, buf, sz);

	return consumed;
}

void hexdump_print_raw_hex(struct hexdump *hd, const char *buf, unsigned int sz) {
	if (hd->out_file) {
		for (size_t i = 0; i < sz; ++i) {
			fprintf(hd->out_file, "%02hhx", buf[i]);

			if ((hd->offset + i) % 16 == 15)
				fprintf(hd->out_file, "\n");
		}

		fflush(hd->out_file);
	}
}


void hexdump_sent_print(struct hexdump *hd, const char *buf, unsigned int sz) {
	if (!sz)
		return;

	if (hd->color_escape)
		printf("%s", hd->color_escape);

	printf("%s -> %s sent %u bytes\n", hd->from, hd->to, sz);

	if (hd->color_escape)
		printf("%s", "\x1b[1m"); /* bold */

	hexdump_print_raw_hex(hd, buf, sz);
	while (sz > 0) {
		printf("%08x  ", hd->offset);

		size_t start_line_offset = (hd->offset >> 4) << 4;
		print_full_hex(hd, start_line_offset, buf, sz);

		printf(" |");
		size_t consumed = print_ascii(hd, start_line_offset, buf, sz);
		printf("|\n");

		buf += consumed;
		sz  -= consumed;

		hd->offset += consumed;
	}

	if (hd->color_escape)
		printf("%s", "\x1b[0m"); /* reset */
	fflush(stdout);
}

void hexdump_remain_print(struct hexdump *hd, unsigned int sz_consumed) {
	if (hd->color_escape)
		printf("%s", hd->color_escape);

	hd->offset_consumer += sz_consumed;

	if (hd->offset_consumer >= hd->offset) {
		printf("%s is in sync\n", hd->to);
	}
	else {
		printf("%s is %i bytes behind\n",
				hd->to, hd->offset - hd->offset_consumer);
	}

	if (hd->color_escape)
		printf("%s", "\x1b[0m"); /* reset */
	fflush(stdout);
}

void hexdump_shutdown_print(struct hexdump *hd) {
	if (hd->color_escape)
		printf("%s", hd->color_escape);

	printf("%s -> %s flow shutdown\n", hd->from, hd->to);

	if (hd->color_escape)
		printf("%s", "\x1b[0m"); /* reset */
	fflush(stdout);
}
