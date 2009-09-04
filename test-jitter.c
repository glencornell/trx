#include <stdio.h>
#include <stdlib.h>

#include "jitter.h"

int main(int argc, char *argv[])
{
	int n;
	struct jitbuf_t jb;
	const char *test = "abcdefghijklmnopqrstuvwxyz1234567890";

	jitbuf_init(&jb);

	for (n = 0; n < strlen(test); n++) {
		int m;
		char *out;
		size_t pos;

		out = jitbuf_front(&jb);
		if (out != NULL)
			fputc(*out, stdout);
		else
			fputc('#', stdout);
		jitbuf_pop(&jb);

		for (m = 0; m < 8; m++) {
			pos = n + rand() % (strlen(test) - n);
			jitbuf_push(&jb, pos, (void*)&test[pos]);
		}
	}
	fputc('\n', stdout);

	jitbuf_clear(&jb);
}
