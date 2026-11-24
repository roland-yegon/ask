#if defined(__unix__) || defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L
#define ASK_POSIX_STREAM_TESTS 1
#endif

#include "ask.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ASK_POSIX_STREAM_TESTS
#include <unistd.h>
#endif

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
    char text[64];
    FILE *in;

    in = input_from("42\n");
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == 42);
    CHECK(ask_last_error() == ASK_OK);
    fclose(in);

    in = input_from("+42\n");
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == 42);
    fclose(in);

    in = input_from("  -17 \n");
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == -17);
    fclose(in);

    in = input_from("00042\n");
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == 42);
    fclose(in);

    snprintf(text, sizeof text, "%d\n", INT_MAX);
    in = input_from(text);
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == INT_MAX);
    fclose(in);

    snprintf(text, sizeof text, "%d\n", INT_MIN);
    in = input_from(text);
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == INT_MIN);
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

    snprintf(text, sizeof text, "%d0\n7\n", INT_MAX);
    in = input_from(text);
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == 7);
    fclose(in);

    snprintf(text, sizeof text, "%ld\n7\n", LONG_MIN);
    in = input_from(text);
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == 7);
    fclose(in);

    in = input_from("");
    CHECK(!ask_int_from(in, NULL, NULL, &value));
    CHECK(ask_last_error() == ASK_EOF);
    fclose(in);

    /* Preserved behaviour: a valid final line at EOF is still an answer. */
    in = input_from("42");
    CHECK(ask_int_from(in, NULL, NULL, &value));
    CHECK(value == 42);
    CHECK(ask_last_error() == ASK_OK);
    fclose(in);

    in = input_from("abc");
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

    snprintf(text, sizeof text, "%ld\n", LONG_MIN);
    in = input_from(text);
    CHECK(ask_long_from(in, NULL, NULL, &value));
    CHECK(value == LONG_MIN);
    fclose(in);

    snprintf(text, sizeof text, "%ld\n", LONG_MAX);
    in = input_from(text);
    CHECK(ask_long_from(in, NULL, NULL, &value));
    CHECK(value == LONG_MAX);
    fclose(in);

    snprintf(text, sizeof text, "%ld0\n7\n", LONG_MAX);
    in = input_from(text);
    CHECK(ask_long_from(in, NULL, NULL, &value));
    CHECK(value == 7);
    fclose(in);

    snprintf(text, sizeof text, "%ld0\n7\n", LONG_MIN);
    in = input_from(text);
    CHECK(ask_long_from(in, NULL, NULL, &value));
    CHECK(value == 7);
    fclose(in);

    in = input_from("12abc\n7\n");
    CHECK(ask_long_from(in, NULL, NULL, &value));
    CHECK(value == 7);
    fclose(in);

    in = input_from("77");
    CHECK(ask_long_from(in, NULL, NULL, &value));
    CHECK(value == 77);
    fclose(in);
}

static void expect_double(const char *answer, double expected)
{
    char text[128];
    double value = -1.0;
    FILE *in;

    snprintf(text, sizeof text, "%s\n", answer);
    in = input_from(text);
    CHECK(ask_double_from(in, NULL, NULL, &value));
    CHECK(value == expected);
    CHECK(ask_last_error() == ASK_OK);
    fclose(in);
}

static void reject_double(const char *bad_answer)
{
    char text[128];
    double value = -1.0;
    FILE *in;

    snprintf(text, sizeof text, "%s\n2.5\n", bad_answer);
    in = input_from(text);
    CHECK(ask_double_from(in, NULL, NULL, &value));
    CHECK(value == 2.5);
    CHECK(ask_last_error() == ASK_OK);
    fclose(in);
}

static void test_ask_double(void)
{
    FILE *in;
    double value = 0.0;

    expect_double("3.14", 3.14);
    expect_double("-2.5e2", -250.0);
    expect_double("1.25E-3", 0.00125);
    expect_double("+1e+2", 100.0);
    expect_double("0", 0.0);
    expect_double(".5", 0.5);
    expect_double("5.", 5.0);
    expect_double(" 7 ", 7.0);

    in = input_from("2.5");
    CHECK(ask_double_from(in, NULL, NULL, &value));
    CHECK(value == 2.5);
    fclose(in);

    /* Decimal-only syntax: no hexadecimal floats. */
    reject_double("0x1p3");
    reject_double("0X1.8P+1");

    /* No special values, including signed and upper-case spellings. */
    reject_double("nan");
    reject_double("NaN");
    reject_double("-nan");
    reject_double("inf");
    reject_double("INF");
    reject_double("+inf");
    reject_double("infinity");
    reject_double("-INFINITY");

    /* Overflow, underflow, malformed syntax and trailing junk. */
    reject_double("1e999");
    reject_double("-1e999");
    reject_double("1e-9999");
    reject_double("1e-400");
    reject_double("12abc");
    reject_double("1.2.3");
    reject_double(".");
    reject_double("e5");
    reject_double("1e");
    reject_double("1e+");
    reject_double("+");
    reject_double("-");
    reject_double("");
    reject_double("   ");
    reject_double("1_000");

    in = input_from("nan\n");
    CHECK(!ask_double_from(in, NULL, NULL, &value));
    CHECK(ask_last_error() == ASK_EOF);
    fclose(in);
}

static void expect_char(const char *answer, char expected)
{
    char text[64];
    char value = '\0';
    FILE *in;

    snprintf(text, sizeof text, "%s\n", answer);
    in = input_from(text);
    CHECK(ask_char_from(in, NULL, NULL, &value));
    CHECK(value == expected);
    CHECK(ask_last_error() == ASK_OK);
    fclose(in);
}

static void reject_char(const char *bad_answer)
{
    char text[128];
    char value = '\0';
    FILE *in;

    snprintf(text, sizeof text, "%s\nA\n", bad_answer);
    in = input_from(text);
    CHECK(ask_char_from(in, NULL, NULL, &value));
    CHECK(value == 'A');
    CHECK(ask_last_error() == ASK_OK);
    fclose(in);
}

static void test_ask_char(void)
{
    FILE *in;
    char value = '\0';

    expect_char("A", 'A');
    expect_char(" A ", 'A');
    expect_char("7", '7');
    expect_char("!", '!');
    expect_char("~", '~');
    expect_char("\tA\t", 'A');

    reject_char("hello");
    reject_char("AB");
    reject_char("A B");
    reject_char("");
    reject_char("   ");
    reject_char("\t");
    reject_char("\xc3\xa9");       /* two-byte UTF-8 sequence */
    reject_char("\xf0\x9f\x99\x82"); /* four-byte UTF-8 sequence */
    reject_char("\x01");           /* non-printable ASCII */
    reject_char("\x7f");           /* DEL is not printable ASCII */

    in = input_from("A");
    CHECK(ask_char_from(in, NULL, NULL, &value));
    CHECK(value == 'A');
    fclose(in);

    in = input_from("hello\n");
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
    CHECK(ask_last_error() == ASK_OK);
    free(line);
    fclose(in);

    in = input_from("windows\r\n");
    line = ask_line_from(in, NULL, NULL);
    CHECK(line != NULL);
    CHECK(strcmp(line, "windows") == 0);
    free(line);
    fclose(in);

    CHECK(ask_line_from(NULL, NULL, NULL) == NULL);
    CHECK(ask_last_error() == ASK_EINVAL);

    /* An empty line is a valid (empty) answer, not EOF. */
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

    /* Embedded NUL bytes terminate the returned C string. */
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
    char cvalue = '\0';
    FILE *in = input_from("bad\n42\n");
    FILE *out = tmpfile();
    FILE *cin = input_from("hello\nA\n");
    FILE *cout = tmpfile();
    const char *transcript;

    CHECK(ask_int_from(in, out, "n> ", &value));
    CHECK(value == 42);
    transcript = capture(out);
    CHECK(strstr(transcript, "n> Expected a whole number.\nn> ") != NULL);
    fclose(in);
    fclose(out);

    CHECK(ask_char_from(cin, cout, "c> ", &cvalue));
    CHECK(cvalue == 'A');
    transcript = capture(cout);
    CHECK(strstr(transcript,
                 "c> Expected exactly one printable ASCII character.\nc> ")
          != NULL);
    fclose(cin);
    fclose(cout);
}

#ifdef ASK_POSIX_STREAM_TESTS
static void test_stream_errors(void)
{
    FILE *in;
    int value = 0;
    char *line;

    /* Reading from a directory succeeds at fopen() but fails at fgetc(). */
    in = fopen("/", "r");
    if (in != NULL) {
        CHECK(ask_line_from(in, NULL, NULL) == NULL);
        CHECK(ask_last_error() == ASK_EIO);
        CHECK(ferror(in) != 0);
        fclose(in);

        in = fopen("/", "r");
        if (in != NULL) {
            CHECK(!ask_int_from(in, NULL, NULL, &value));
            CHECK(ask_last_error() == ASK_EIO);
            fclose(in);
        }
    }

    /* Bytes buffered before an I/O error must not become a "successful"
     * partial line. Closing the underlying descriptor forces EBADF. */
    in = tmpfile();
    if (in != NULL) {
        fputs("partial", in);
        fflush(in);
        rewind(in);
        (void)fgetc(in);          /* prime the FILE buffer */
        (void)close(fileno(in));  /* subsequent reads now fail */
        line = ask_line_from(in, NULL, NULL);
        CHECK(line == NULL);
        CHECK(ask_last_error() == ASK_EIO);
        CHECK(ferror(in) != 0);
        fclose(in);
    }
}
#endif

static void test_fuzz_parsers(void)
{
    static const char alphabet[] = "0123456789+-.eExXpPabc \t";
    unsigned long seed = 12345UL;
    char line[32];
    int i;

    for (i = 0; i < 300; ++i) {
        size_t len;
        size_t j;
        int ivalue = 0;
        long lvalue = 0;
        double dvalue = 0.0;
        char cvalue = '\0';
        FILE *in;

        seed = seed * 1103515245UL + 12345UL;
        len = (size_t)((seed >> 16) % 20UL);
        for (j = 0; j < len; ++j) {
            seed = seed * 1103515245UL + 12345UL;
            line[j] = alphabet[(seed >> 16) % (sizeof alphabet - 1U)];
        }
        line[len] = '\0';

        in = input_from(line);
        (void)ask_int_from(in, NULL, NULL, &ivalue);
        CHECK(ask_last_error() == ASK_OK || ask_last_error() == ASK_EOF);
        fclose(in);

        in = input_from(line);
        (void)ask_long_from(in, NULL, NULL, &lvalue);
        CHECK(ask_last_error() == ASK_OK || ask_last_error() == ASK_EOF);
        fclose(in);

        in = input_from(line);
        (void)ask_double_from(in, NULL, NULL, &dvalue);
        CHECK(ask_last_error() == ASK_OK || ask_last_error() == ASK_EOF);
        fclose(in);

        in = input_from(line);
        if (ask_char_from(in, NULL, NULL, &cvalue)) {
            CHECK((unsigned char)cvalue >= 0x21 &&
                  (unsigned char)cvalue <= 0x7e);
        } else {
            CHECK(ask_last_error() == ASK_EOF);
        }
        fclose(in);
    }
}

static void test_error_strings(void)
{
    CHECK(strcmp(ask_error_string(ASK_OK), "success") == 0);
    CHECK(strcmp(ask_error_string(ASK_EOF), "end of input") == 0);
    CHECK(strcmp(ask_error_string(ASK_NOMEM), "out of memory") == 0);
    CHECK(strcmp(ask_error_string(ASK_EINVAL), "invalid argument") == 0);
    CHECK(strcmp(ask_error_string(ASK_EIO), "stream error") == 0);
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
#ifdef ASK_POSIX_STREAM_TESTS
    test_stream_errors();
#endif
    test_fuzz_parsers();
    test_error_strings();

    if (failures != 0) {
        fprintf(stderr, "%d test check(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    puts("All checks passed.");
    return EXIT_SUCCESS;
}
