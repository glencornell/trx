#ifndef JITTER_H
#define JITTER_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>

#define JITBUF_MAX 64

typedef unsigned int seq_t;

/*
 * De-jitter buffer
 */

struct jitbuf_t {
	seq_t base;
	size_t out, entries, contiguous;
	void *buf[JITBUF_MAX];
};

void jitbuf_init(struct jitbuf_t *jb);
void jitbuf_clear(struct jitbuf_t *jb);

int jitbuf_push(struct jitbuf_t *jb, seq_t t, void *data);
void* jitbuf_front(const struct jitbuf_t *jb);
void jitbuf_pop(struct jitbuf_t *jb);

bool jitbuf_ready(const struct jitbuf_t *jb, seq_t count);
bool jitbuf_empty(const struct jitbuf_t *jb);

void jitbuf_debug(struct jitbuf_t *jb, FILE *fd);

#endif
