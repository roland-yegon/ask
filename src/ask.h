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
    ASK_EOF,      /* end of input before an answer was available           */
    ASK_NOMEM,    /* memory allocation failed                              */
    ASK_EINVAL,   /* invalid argument (NULL stream or result pointer)      */
    ASK_EIO       /* input stream reported a read error                    */
} ask_error;

/* Error state of the most recent ask_* read in the calling thread.
 * It is thread-local; ask_error_string() does not change it. */
ask_error ask_last_error(void);

/* Human-readable description, e.g. "stream error". Never returns NULL;
 * unknown values map to "unknown error". */
const char *ask_error_string(ask_error error);

/* ------------------------------------------------------------------------- *
 * High-level API - reads from stdin, writes prompts to stdout
 * ------------------------------------------------------------------------- */

/* Each typed function keeps prompting until the input is valid.
 * Returns true and stores the result in *out on success, resetting the
 * error state to ASK_OK. Returns false on EOF, stream error, invalid
 * arguments, or allocation failure; ask_last_error() says which.
 * Invalid user input is not an error: it triggers another prompt.
 *
 * Accepted forms:
 *   ask_int    a base-10 integer in the range of int; a leading sign and
 *              surrounding whitespace are allowed
 *   ask_long   a base-10 integer in the range of long
 *   ask_double a finite decimal floating-point value in the current C
 *              locale; decimal and scientific notation are allowed,
 *              hexadecimal floats, nan, inf, overflow and underflow are not
 *   ask_char   exactly one printable ASCII character (0x21..0x7e),
 *              surrounded by optional whitespace; multi-byte UTF-8 input
 *              and multiple non-whitespace characters are rejected
 *
 * The decimal separator is taken from localeconv() for the current locale;
 * this library never calls setlocale(). In the default "C" locale it is '.'.
 */
bool ask_int(const char *prompt, int *out);
bool ask_long(const char *prompt, long *out);
bool ask_double(const char *prompt, double *out);
bool ask_char(const char *prompt, char *out);

/* Reads a whole line of any length. The buffer grows as the user types.
 * Returns a malloc'd string (no trailing newline or CR) that the caller
 * owns and must release with free(), or NULL on EOF with nothing typed,
 * stream error, invalid stream, or allocation failure. On EOF after some
 * characters were read, those characters are returned as a normal line.
 * On allocation failure the partial line is discarded so that a later read
 * starts at the next line. On stream error the stream's error indicator is
 * left set for the caller. Text is treated as bytes: an embedded NUL byte
 * terminates the C string returned to the caller.
 */
char *ask_line(const char *prompt);

/* ------------------------------------------------------------------------- *
 * Advanced API - same behaviour, but with caller-supplied streams.
 * `in` must be readable, `out` may be NULL to suppress prompts. These
 * functions use their streams only for the duration of the call; callers
 * using one stream from several threads must serialize access themselves.
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
