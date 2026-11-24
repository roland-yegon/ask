#ifndef ASK_H
#define ASK_H

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- *
 * Error reporting
 * ------------------------------------------------------------------------- */

typedef enum {
    ASK_OK = 0,   /* last operation succeeded                              */
    ASK_EOF,      /* end of input (Ctrl+D) before an answer was available  */
    ASK_NOMEM,    /* memory allocation failed                              */
    ASK_EINVAL    /* invalid argument (NULL stream or output pointer)      */
} ask_error;

/* Error state of the most recent call. It is thread-local. */
ask_error ask_last_error(void);

/* Human-readable description, e.g. "end of input". Never returns NULL. */
const char *ask_error_string(ask_error error);

/* ------------------------------------------------------------------------- *
 * High-level API — reads from stdin, writes prompts to stdout
 * ------------------------------------------------------------------------- */

/* Each function keeps prompting until the input is valid.
 * Returns true and stores the result in *out on success.
 * Returns false on EOF, invalid arguments, or allocation failure;
 * call ask_last_error() to find out which.
 *
 * Accepted forms:
 *   ask_int    any base-10 integer that fits in int (leading sign and
 *              surrounding whitespace allowed)
 *   ask_long   any base-10 integer that fits in long
 *   ask_double any finite base-10 floating-point value that does not
 *              overflow or underflow (scientific notation allowed)
 *   ask_char   the first non-whitespace character of a non-empty line;
 *              anything after that character is ignored
 */
bool ask_int(const char *prompt, int *out);
bool ask_long(const char *prompt, long *out);
bool ask_double(const char *prompt, double *out);
bool ask_char(const char *prompt, char *out);

/* Reads a whole line of any length. The buffer grows as the user types.
 * Returns a malloc'd string (no trailing newline or CR) that the caller
 * must free(), or NULL on EOF with nothing typed, invalid stream, or
 * allocation failure. Text is treated as bytes: an embedded NUL byte
 * terminates the C string returned to the caller.
 */
char *ask_line(const char *prompt);

/* ------------------------------------------------------------------------- *
 * Advanced API — same behaviour, but with caller-supplied streams.
 * Useful for sockets, files, test doubles, or redirecting output.
 * ------------------------------------------------------------------------- */

bool ask_int_from(FILE *in, FILE *out, const char *prompt, int *value);
bool ask_long_from(FILE *in, FILE *out, const char *prompt, long *value);
bool ask_double_from(FILE *in, FILE *out, const char *prompt, double *value);
bool ask_char_from(FILE *in, FILE *out, const char *prompt, char *value);
char *ask_line_from(FILE *in, FILE *out, const char *prompt);

#ifdef __cplusplus
}
#endif

#endif /* ASK_H */
