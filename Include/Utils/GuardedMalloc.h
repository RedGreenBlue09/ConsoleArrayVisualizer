#pragma once

#include <stdlib.h>

static inline void* malloc_guarded(size_t size) {
	void* p = malloc(size);
	if (!p) abort();
	return p;
}

static inline void* calloc_guarded(size_t count, size_t size) {
	void* p = calloc(count, size);
	if (!p) abort();
	return p;
}

static inline void* realloc_guarded(void* p, size_t size) {
	void* pNew = realloc(p, size);
	if (!pNew) {
		free(p);
		abort();
	}
	return pNew;
}
