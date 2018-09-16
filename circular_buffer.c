#include "circular_buffer.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

int circular_buffer_init(struct circular_buffer_t *b, size_t sz) {
	memset(b, 0, sizeof(*b));
	b->sz = sz;
	b->buf = (char*)malloc(sz);
	if (!b->buf)
		return -1;

	return 0;
}

void circular_buffer_destroy(struct circular_buffer_t *b) {
	free(b->buf);
}

size_t circular_buffer_get_free(struct circular_buffer_t *b) {
	return b->hbehind? (b->tail - b->head) : (b->sz - b->head);
}

size_t circular_buffer_get_ready(struct circular_buffer_t *b) {
	return b->hbehind? (b->sz - b->tail) : (b->head - b->tail);
}

void circular_buffer_advance_head(struct circular_buffer_t *b, size_t s) {
	assert (s <= circular_buffer_get_free(b));
	b->head += s;
	if (b->head == b->sz) {
		b->head = 0;
		b->hbehind = true;
	}
}

void circular_buffer_advance_tail(struct circular_buffer_t *b, size_t s) {
	assert (s <= circular_buffer_get_ready(b));
	b->tail += s;
	if (b->tail == b->sz) {
		b->tail = 0;
		b->hbehind = false;
	}
}
