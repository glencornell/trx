#include <assert.h>
#include <stdlib.h>

#include "jitter.h"

void jitbuf_init(struct jitbuf_t *jb)
{
	size_t n;

	for (n = 0; n < JITBUF_MAX; n++)
		jb->buf[n] = NULL;

	jb->base = 0;
	jb->out = 0;
	jb->entries = 0;
}

void jitbuf_clear(struct jitbuf_t *jb)
{
}

/*
 * Push an entry into the buffer, with the given sequence number
 *
 * Pre: data is not NULL
 */

int jitbuf_push(struct jitbuf_t *jb, seq_t t, void *data)
{
	void **entry;

	assert(data != NULL);

	if (jb->entries == 0 && t != jb->base) {
		fprintf(stderr, "Restarting (%d)\n", t);
		jb->base = t;
	} else {
		if (t < jb->base) {
			fprintf(stderr, "Late (%d)\n", t);
			return -1;
		}
		if (t >= jb->base + JITBUF_MAX) {
			fprintf(stderr, "Early (%d)\n", t);
			return -1;
		}
	}

	entry = &jb->buf[(t - jb->base + jb->out) % JITBUF_MAX];

	if (*entry != NULL) {
		fprintf(stderr, "Dup (%d)\n", t);
		return -1;
	}

	*entry = data;
	jb->entries++;

	return 0;
}

/*
 * Get the entry at the front of the buffer, in sequence number order
 *
 * Return: NULL if no entry, otherwise entry next in sequence
 */

void* jitbuf_front(const struct jitbuf_t *jb)
{
	return jb->buf[jb->out];
}

/*
 * Pop the next entry in the buffer
 */

void jitbuf_pop(struct jitbuf_t *jb)
{
	void **entry;

	entry = &jb->buf[jb->out];
	if (*entry != NULL) {
		jb->entries--;
		*entry = NULL;
	}
	jb->base++;
	jb->out++;
	if (jb->out >= JITBUF_MAX)
		jb->out = 0;
}

/*
 * Return true if this buffer has enough data flow to be declared ready
 */

bool jitbuf_ready(const struct jitbuf_t *jb, seq_t count)
{
	size_t n;

	assert(count <= JITBUF_MAX);

	for (n = 0; n < count; n++) {
		if (jb->buf[(jb->out + n) % JITBUF_MAX] == NULL)
			return false;
	}
	return true;
}

bool jitbuf_empty(const struct jitbuf_t *jb)
{
	if (jb->entries > 0)
		return false;
	else
		return true;
}

void jitbuf_debug(struct jitbuf_t *jb, FILE *fd)
{
	seq_t n;

	fprintf(fd, "% 3d ", jb->entries);

	for (n = 0; n < JITBUF_MAX; n++) {
		if (jb->buf[(jb->out + n) % JITBUF_MAX] != NULL) {
			fputc('#', fd);
		} else {
			fputc(' ', fd);
		}
	}
	fputc('\n', fd);
}
