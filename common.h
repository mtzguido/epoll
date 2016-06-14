#ifndef _COMMON_H
#define _COMMON_H

#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>

typedef struct sockaddr_in sin;
typedef struct sockaddr    sad;

static void die(char *s, ...) {
	va_list v;

	va_start(v, s);
	vfprintf(stderr, s, v);
	fprintf(stderr, "\n");
	va_end(v);
	fprintf(stderr, " -- errno = %i (%m)\n", errno);

	fflush(stderr);
	abort();
}

#endif
