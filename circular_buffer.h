
#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <stdlib.h>
#include <stdbool.h>

struct circular_buffer_t {
	char *buf;
	size_t head;
	size_t tail;
	bool full;
	size_t sz;
};

int circular_buffer_init(struct circular_buffer_t *b, size_t sz);
void circular_buffer_destroy(struct circular_buffer_t *b);

size_t circular_buffer_get_free(struct circular_buffer_t *b);
size_t circular_buffer_get_ready(struct circular_buffer_t *b);

void circular_buffer_advance_head(struct circular_buffer_t *b, size_t s);
void circular_buffer_advance_tail(struct circular_buffer_t *b, size_t s);

/*
 *    /- head/tail
 *   V
 *   +--------------------------+
 *   |                          |
 *   +--------------------------+
 *   full  = false
 *   free  = sz - head
 *   ready = head - tail
 *   assert ( head >= tail )
 *
 *
 *    /- tail      /- head
 *   V            V
 *   +--------------------------+
 *   |:::::::::::::             |
 *   +--------------------------+
 *   full  = false
 *   free  = sz - head
 *   ready = head - tail
 *   assert ( head >= tail )
 *
 *
 *                  /- tail      /- head
 *                 V            V
 *   +--------------------------+
 *   |             :::::::::::::|
 *   +--------------------------+
 *   full  = false
 *   free  = sz - head
 *   ready = head - tail
 *   assert ( head >= tail )
 *
 *
 *    /- head       /- tail
 *   V             V
 *   +--------------------------+
 *   |             :::::::::::::|
 *   +--------------------------+
 *   full  = true
 *   free  = tail - head
 *   ready = sz - tail
 *   assert ( tail >= head )
 *
 *
 *                /- head        /- tail
 *               V              V
 *   +--------------------------+
 *   |:::::::::::               |
 *   +--------------------------+
 *   full  = true
 *   free  = tail - head
 *   ready = sz - tail
 *   assert ( tail >= head )
 *
 *
 *    /- tail      /- head
 *   V            V
 *   +--------------------------+
 *   |:::::::::::               |
 *   +--------------------------+
 *   full  = false
 *   free  = sz - head
 *   ready = head - tail
 *   assert ( head >= tail )
 *
 **/

#endif
