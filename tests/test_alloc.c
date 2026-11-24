#include "test_alloc.h"

#include "ask.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int live_allocations;
static int fail_next_malloc;
static int fail_next_realloc;
static int failures;

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr);    \
            failures++;                                                        \
        }                                                                      \
    } while (0)

void *ask_test_malloc(size_t size)
{
    void *p;

    if (fail_next_malloc) {
        fail_next_malloc = 0;
        return NULL;
    }
    p = malloc(size);
    if (p != NULL) {
        live_allocations++;
    }
    return p;
}

void *ask_test_realloc(void *ptr, size_t size)
{
    void *p;

    if (fail_next_realloc) {
        fail_next_realloc = 0;
        return NULL; /* caller still owns ptr */
    }
    p = realloc(ptr, size);
    if (p != NULL && ptr == NULL) {
        live_allocations++;
    }
    return p;
}

void ask_test_free(void *ptr)
{
    if (ptr != NULL) {
        live_allocations--;
        free(ptr);
    }
}

static FILE *input_from(const char *text)
{
    FILE *f = tmpfile();
    if (f == NULL) {
        fputs("tmpfile() failed\n", stderr);
        exit(EXIT_FAILURE);
    }
    fputs(text, f);
    rewind(f);
    return f;
}

static void test_initial_malloc_failure(void)
{
    FILE *in = input_from("hello\n");
    char *line;

    fail_next_malloc = 1;
    CHECK(ask_line_from(in, NULL, NULL) == NULL);
    CHECK(ask_last_error() == ASK_NOMEM);
    CHECK(live_allocations == 0);

    /* No bytes were consumed before the failed allocation. */
    line = ask_line_from(in, NULL, NULL);
    CHECK(line != NULL);
    CHECK(strcmp(line, "hello") == 0);
    ask_test_free(line);
    CHECK(live_allocations == 0);
    fclose(in);
}

static void test_realloc_failure(void)
{
    char *long_line = malloc(5002);
    FILE *in;

    CHECK(long_line != NULL);
    memset(long_line, 'x', 5000);
    long_line[5000] = '\n';
    long_line[5001] = '\0';
    in = input_from(long_line);

    fail_next_realloc = 1;
    CHECK(ask_line_from(in, NULL, NULL) == NULL);
    CHECK(ask_last_error() == ASK_NOMEM);
    CHECK(live_allocations == 0);

    /* The rest of the failed line was drained, so the next read sees EOF. */
    CHECK(ask_line_from(in, NULL, NULL) == NULL);
    CHECK(ask_last_error() == ASK_EOF);
    CHECK(live_allocations == 0);

    free(long_line);
    fclose(in);
}

static void test_successful_paths(void)
{
    int value = 0;
    FILE *in = input_from("ok\n");
    char *line = ask_line_from(in, NULL, NULL);

    CHECK(line != NULL);
    CHECK(strcmp(line, "ok") == 0);
    CHECK(live_allocations == 1);
    ask_test_free(line);
    CHECK(live_allocations == 0);
    fclose(in);

    in = input_from("42\n");
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == 42);
    CHECK(ask_last_error() == ASK_OK);
    CHECK(live_allocations == 0); /* retry loop freed its line */
    fclose(in);

    in = input_from("tail without newline");
    line = ask_line_from(in, NULL, NULL);
    CHECK(line != NULL);
    CHECK(strcmp(line, "tail without newline") == 0);
    ask_test_free(line);
    CHECK(live_allocations == 0);
    fclose(in);
}

int main(void)
{
    test_initial_malloc_failure();
    test_realloc_failure();
    test_successful_paths();

    if (failures != 0) {
        fprintf(stderr, "%d allocation check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("Allocation checks passed.");
    return EXIT_SUCCESS;
}
