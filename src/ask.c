#include "ask.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define ASK_THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
#define ASK_THREAD_LOCAL __declspec(thread)
#else
#define ASK_THREAD_LOCAL
#endif

static ASK_THREAD_LOCAL ask_error last_error = ASK_OK;

ask_error ask_last_error(void)
{
    return last_error;
}

const char *ask_error_string(ask_error error)
{
    switch (error) {
    case ASK_OK:    return "success";
    case ASK_EOF:   return "end of input";
    case ASK_NOMEM: return "out of memory";
    case ASK_EINVAL: return "invalid argument";
    default:        return "unknown error";
    }
}

/* ------------------------------------------------------------------------- *
 * Line reading
 * ------------------------------------------------------------------------- */

/* Discard the rest of a line after an allocation failure so that a later
 * read does not silently consume the user's leftover keystrokes. */
static void discard_rest_of_line(FILE *in)
{
    int c;
    while ((c = fgetc(in)) != EOF && c != '\n') {
        /* nothing */
    }
}

char *ask_line_from(FILE *in, FILE *out, const char *prompt)
{
    size_t cap = 32;
    size_t len = 0;
    char *buf;
    int c = EOF;

    if (in == NULL) {
        last_error = ASK_EINVAL;
        return NULL;
    }

    buf = malloc(cap);
    if (buf == NULL) {
        last_error = ASK_NOMEM;
        return NULL;
    }

    if (prompt != NULL && out != NULL) {
        fputs(prompt, out);
        fflush(out);
    }

    while ((c = fgetc(in)) != EOF && c != '\n') {
        /* Keep room for the terminating '\0'. */
        if (len + 1 >= cap) {
            char *tmp;
            if (cap > SIZE_MAX / 2) {
                free(buf);
                discard_rest_of_line(in);
                last_error = ASK_NOMEM;
                return NULL;
            }
            tmp = realloc(buf, cap * 2);
            if (tmp == NULL) {
                free(buf);
                discard_rest_of_line(in);
                last_error = ASK_NOMEM;
                return NULL;
            }
            buf = tmp;
            cap *= 2;
        }
        buf[len++] = (char)c;
    }

    /* EOF with nothing typed means there is no one left to ask. */
    if (c == EOF && len == 0) {
        free(buf);
        last_error = ASK_EOF;
        return NULL;
    }

    /* Tolerate Windows-style CRLF input. */
    if (len > 0 && buf[len - 1] == '\r') {
        len--;
    }

    buf[len] = '\0';
    last_error = ASK_OK;
    return buf;
}

char *ask_line(const char *prompt)
{
    return ask_line_from(stdin, stdout, prompt);
}

/* ------------------------------------------------------------------------- *
 * Validation helpers
 * ------------------------------------------------------------------------- */

static const char *skip_whitespace(const char *s)
{
    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

static bool only_whitespace_remains(const char *s)
{
    for (; *s != '\0'; s++) {
        if (!isspace((unsigned char)*s)) {
            return false;
        }
    }
    return true;
}

static bool parse_long(const char *text, long *value)
{
    const char *start = skip_whitespace(text);
    char *end;

    if (*start == '\0') {
        return false;
    }

    errno = 0;
    *value = strtol(start, &end, 10);
    if (end == start || errno == ERANGE || !only_whitespace_remains(end)) {
        return false;
    }
    return true;
}

static bool parse_int_value(const char *text, void *out)
{
    long value;
    if (!parse_long(text, &value) || value < INT_MIN || value > INT_MAX) {
        return false;
    }
    *(int *)out = (int)value;
    return true;
}

static bool parse_long_value(const char *text, void *out)
{
    return parse_long(text, (long *)out);
}

static bool parse_double_value(const char *text, void *out)
{
    const char *start = skip_whitespace(text);
    char *end;
    double value;

    if (*start == '\0') {
        return false;
    }

    errno = 0;
    value = strtod(start, &end);
    if (end == start || !only_whitespace_remains(end) || !isfinite(value)) {
        return false;
    }
    /* strtod sets ERANGE for overflow and for underflow; neither is a
     * usable, exactly representable answer here. */
    if (errno == ERANGE) {
        return false;
    }

    *(double *)out = value;
    return true;
}

static bool parse_char_value(const char *text, void *out)
{
    const char *start = skip_whitespace(text);
    if (*start == '\0') {
        return false;
    }
    *(char *)out = *start;
    return true;
}

typedef bool (*parse_fn)(const char *text, void *out);

/* The retry loop shared by every typed ask_* function. */
static bool ask_value(FILE *in, FILE *out, const char *prompt,
                      parse_fn parse, void *value, const char *error_message)
{
    if (in == NULL || value == NULL) {
        last_error = ASK_EINVAL;
        return false;
    }

    for (;;) {
        char *line = ask_line_from(in, out, prompt);
        bool valid;

        if (line == NULL) {
            return false; /* ask_line_from has already set last_error */
        }

        valid = parse(line, value);
        free(line);

        if (valid) {
            last_error = ASK_OK;
            return true;
        }

        if (out != NULL) {
            fputs(error_message, out);
            fputc('\n', out);
            fflush(out);
        }
    }
}

bool ask_int_from(FILE *in, FILE *out, const char *prompt, int *value)
{
    return ask_value(in, out, prompt, parse_int_value, value,
                     "Expected a whole number.");
}

bool ask_long_from(FILE *in, FILE *out, const char *prompt, long *value)
{
    return ask_value(in, out, prompt, parse_long_value, value,
                     "Expected a whole number.");
}

bool ask_double_from(FILE *in, FILE *out, const char *prompt, double *value)
{
    return ask_value(in, out, prompt, parse_double_value, value,
                     "Expected a number.");
}

bool ask_char_from(FILE *in, FILE *out, const char *prompt, char *value)
{
    return ask_value(in, out, prompt, parse_char_value, value,
                     "Expected at least one character.");
}

bool ask_int(const char *prompt, int *out)
{
    return ask_int_from(stdin, stdout, prompt, out);
}

bool ask_long(const char *prompt, long *out)
{
    return ask_long_from(stdin, stdout, prompt, out);
}

bool ask_double(const char *prompt, double *out)
{
    return ask_double_from(stdin, stdout, prompt, out);
}

bool ask_char(const char *prompt, char *out)
{
    return ask_char_from(stdin, stdout, prompt, out);
}
