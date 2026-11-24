#include "ask.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr);    \
            failures++;                                                        \
        }                                                                      \
    } while (0)

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

static char *capture(FILE *stream)
{
    static char buf[8192];
    size_t len = 0;
    int c;

    rewind(stream);
    while ((c = fgetc(stream)) != EOF && len + 1 < sizeof buf) {
        buf[len++] = (char)c;
    }
    buf[len] = '\0';
    return buf;
}

static void test_ask_int(void)
{
    int value = 0;
    FILE *in;

    in = input_from("42\n");
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == 42);
    CHECK(ask_last_error() == ASK_OK);
    fclose(in);

    in = input_from("  -17 \n");
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == -17);
    fclose(in);

    in = input_from("oops\n3.5\n999999999999999999999\n21\n");
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == 21);
    CHECK(ask_last_error() == ASK_OK);
    fclose(in);

    in = input_from("12abc\n7\n");
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == 7);
    fclose(in);

    in = input_from("");
    CHECK(!ask_int_from(in, NULL, NULL, &value));
    CHECK(ask_last_error() == ASK_EOF);
    fclose(in);

    in = input_from("5\n");
    CHECK(!ask_int_from(in, NULL, NULL, NULL));
    CHECK(ask_last_error() == ASK_EINVAL);
    CHECK(!ask_int_from(NULL, NULL, NULL, &value));
    CHECK(ask_last_error() == ASK_EINVAL);
    fclose(in);
}

static void test_ask_long(void)
{
    long value = 0;
    char text[128];
    FILE *in;

    in = input_from("123456789\n");
    CHECK(ask_long_from(in, NULL, NULL, &value));
    CHECK(value == 123456789L);
    fclose(in);

    snprintf(text, sizeof text, "%ld\n", LONG_MAX);
    in = input_from(text);
    CHECK(ask_long_from(in, NULL, NULL, &value));
    CHECK(value == LONG_MAX);
    fclose(in);

    snprintf(text, sizeof text, "%ld0\n", LONG_MAX);
    in = input_from(text);
    CHECK(!ask_long_from(in, NULL, NULL, &value));
    CHECK(ask_last_error() == ASK_EOF);
    fclose(in);
}

static void test_ask_double(void)
{
    double value = 0.0;
    FILE *in;

    in = input_from("3.5\n");
    CHECK(ask_double_from(in, NULL, NULL, &value));
    CHECK(value == 3.5);
    fclose(in);

    in = input_from(" -2.5e2 \n");
    CHECK(ask_double_from(in, NULL, NULL, &value));
    CHECK(value == -250.0);
    fclose(in);

    in = input_from("nan\ninf\n1e999\n2.5\n");
    CHECK(ask_double_from(in, NULL, NULL, &value));
    CHECK(value == 2.5);
    fclose(in);
}

static void test_ask_char(void)
{
    char value = '\0';
    FILE *in;

    in = input_from("  y\n");
    CHECK(ask_char_from(in, NULL, NULL, &value));
    CHECK(value == 'y');
    fclose(in);

    in = input_from("\n \t\nz extra\n");
    CHECK(ask_char_from(in, NULL, NULL, &value));
    CHECK(value == 'z');
    fclose(in);

    in = input_from("\n  \n");
    CHECK(!ask_char_from(in, NULL, NULL, &value));
    CHECK(ask_last_error() == ASK_EOF);
    fclose(in);
}

static void test_ask_line(void)
{
    char *line;
    FILE *in;
    FILE *out;
    char *long_line;

    in = input_from("Ada\n");
    out = tmpfile();
    line = ask_line_from(in, out, "name> ");
    CHECK(line != NULL);
    CHECK(strcmp(line, "Ada") == 0);
    CHECK(strcmp(capture(out), "name> ") == 0);
    CHECK(ask_last_error() == ASK_OK);
    free(line);
    fclose(in);
    fclose(out);

    in = input_from("");
    CHECK(ask_line_from(in, NULL, NULL) == NULL);
    CHECK(ask_last_error() == ASK_EOF);
    fclose(in);

    in = input_from("tail without newline");
    line = ask_line_from(in, NULL, NULL);
    CHECK(line != NULL);
    CHECK(strcmp(line, "tail without newline") == 0);
    free(line);
    fclose(in);

    in = input_from("windows\r\n");
    line = ask_line_from(in, NULL, NULL);
    CHECK(line != NULL);
    CHECK(strcmp(line, "windows") == 0);
    free(line);
    fclose(in);

    in = input_from("");
    CHECK(ask_line_from(NULL, NULL, NULL) == NULL);
    CHECK(ask_last_error() == ASK_EINVAL);
    fclose(in);

    /* The empty line is a valid (empty) answer, not EOF. */
    in = input_from("\nnext\n");
    line = ask_line_from(in, NULL, NULL);
    CHECK(line != NULL);
    CHECK(line[0] == '\0');
    free(line);
    line = ask_line_from(in, NULL, NULL);
    CHECK(line != NULL);
    CHECK(strcmp(line, "next") == 0);
    free(line);
    fclose(in);

    /* Growth beyond the initial capacity. */
    long_line = malloc(5002);
    CHECK(long_line != NULL);
    memset(long_line, 'x', 5000);
    long_line[5000] = '\n';
    long_line[5001] = '\0';
    in = input_from(long_line);
    line = ask_line_from(in, NULL, NULL);
    CHECK(line != NULL);
    CHECK(strlen(line) == 5000);
    free(line);
    free(long_line);
    fclose(in);

    /* Embedded NUL bytes are treated as text and terminate the C string. */
    in = input_from("ab\0cd\n");
    line = ask_line_from(in, NULL, NULL);
    CHECK(line != NULL);
    CHECK(strlen(line) == 2);
    free(line);
    fclose(in);
}

static void test_retry_output(void)
{
    int value = 0;
    FILE *in = input_from("bad\n42\n");
    FILE *out = tmpfile();
    const char *transcript;

    CHECK(ask_int_from(in, out, "n> ", &value));
    CHECK(value == 42);
    transcript = capture(out);
    CHECK(strstr(transcript, "n> Expected a whole number.\nn> ") != NULL);
    CHECK(strstr(transcript, "Expected a whole number.") != NULL);
    fclose(in);
    fclose(out);
}

static void test_error_strings(void)
{
    CHECK(strcmp(ask_error_string(ASK_OK), "success") == 0);
    CHECK(strcmp(ask_error_string(ASK_EOF), "end of input") == 0);
    CHECK(strcmp(ask_error_string(ASK_NOMEM), "out of memory") == 0);
    CHECK(strcmp(ask_error_string(ASK_EINVAL), "invalid argument") == 0);
    CHECK(strcmp(ask_error_string((ask_error)999), "unknown error") == 0);
}

int main(void)
{
    test_ask_int();
    test_ask_long();
    test_ask_double();
    test_ask_char();
    test_ask_line();
    test_retry_output();
    test_error_strings();

    if (failures != 0) {
        fprintf(stderr, "%d test check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("All checks passed.");
    return EXIT_SUCCESS;
}
