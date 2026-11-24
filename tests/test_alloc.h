#ifndef TEST_ALLOC_H
#define TEST_ALLOC_H

#include <stddef.h>

/* Failure-injection allocators used by tests/test_alloc.c. */
void *ask_test_malloc(size_t size);
void *ask_test_realloc(void *ptr, size_t size);
void ask_test_free(void *ptr);

#endif /* TEST_ALLOC_H */
