<div align="center">

<img src="docs/banner.svg" alt="ask: validated user input for C" width="720">

<br>

<a href="https://github.com/roland-yegon/ask/actions/workflows/ci.yml"><img src="https://github.com/roland-yegon/ask/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
<a href="https://en.wikipedia.org/wiki/C11_(C_standard_revision)"><img src="https://img.shields.io/badge/C-C11-00599C.svg?logo=c&logoColor=white" alt="C11"></a>
<img src="https://img.shields.io/badge/dependencies-zero-brightgreen.svg" alt="Zero dependencies">
<a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-green.svg" alt="MIT license"></a>
<a href="https://github.com/roland-yegon/ask/pulls"><img src="https://img.shields.io/badge/PRs-welcome-ff69b4.svg" alt="PRs welcome"></a>

# ask

A small C11 library for validated user input.

[Introduction](#introduction) | [Usage](#usage-examples) | [API](#complete-api-reference) | [Validation](#input-validation-rules) | [Build and test](#build-and-test-instructions)

</div>

## Introduction

`ask` prints a prompt, reads a whole line, validates it, and asks again when the answer is invalid. It provides typed readers for `int`, `long`, `double`, and `char`, plus a dynamically sized line reader with an explicit ownership contract.

The library uses only the C standard library. It never calls `setlocale()` and keeps error state in thread-local storage.

## Problem it solves

Validated interactive input in plain C normally requires a drain loop to discard unmatched characters after a failed conversion:

```c
int age;
int rc;
do {
    printf("Age? ");
    rc = scanf("%d", &age);
    if (rc != 1) {
        puts("Expected a whole number.");
        while (getchar() != '\n' && !feof(stdin)) {
            /* discard the unmatched input */
        }
    }
} while (rc != 1);
```

With `ask`, each attempt consumes a complete line, so invalid input cannot poison the next read:

```c
#include "ask.h"

int age;
ask_int("Age? ", &age);
```

## Main features

| Feature | Description |
| --- | --- |
| Retry loops | Typed readers re-prompt on invalid syntax or out-of-range values |
| Four typed readers | `ask_int`, `ask_long`, `ask_double`, `ask_char` |
| Dynamic line reader | `ask_line` grows its buffer to fit the input |
| Injectable streams | Every function has a `*_from(FILE *, FILE *, ...)` variant |
| Explicit error states | EOF, allocation failure, bad arguments, and stream errors are distinguished |
| Small footprint | Two source files in `src/`; no generated code or external packages |
| Automated checks | GCC and Clang builds with warnings as errors, sanitizer runs, allocation-failure tests |

## Requirements and compatibility

- A C11 compiler and a conforming C standard library.
- Built and tested on Linux with GCC and Clang in CI.
- The code is written in standard C11, but Windows and macOS are not currently covered by CI, so no support claim is made for them.
- No external dependencies. The library never changes the process locale.
- C++ users can include the header because declarations are wrapped in `extern "C"`.

## Installation

```bash
git clone https://github.com/roland-yegon/ask.git
cd ask
make
sudo make install
```

`make install` places `ask.h` in `$(PREFIX)/include` and `libask.a` in `$(PREFIX)/lib`; the default prefix is `/usr/local`.

To use an installed copy:

```bash
cc -std=c11 app.c -lask -o app
```

To compile against a local checkout without installing:

```bash
cc -std=c11 -Iask/src app.c ask/build/libask.a -o app
```

## Build instructions

```bash
make            # builds build/libask.a and build/basic
make lib        # static library only
make example    # example program only
make clean      # removes build/
make install PREFIX=~/.local
```

## Usage examples

```c
#include "ask.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int age;
    long visits;
    double score;
    char initial;

    if (!ask_int("Age? ", &age) ||
        !ask_long("Lifetime visits? ", &visits) ||
        !ask_double("Score? ", &score) ||
        !ask_char("Initial (one character)? ", &initial)) {
        fprintf(stderr, "Could not read input: %s\n",
                ask_error_string(ask_last_error()));
        return EXIT_FAILURE;
    }

    char *name = ask_line("Name? ");
    if (name == NULL) {
        fprintf(stderr, "Could not read input: %s\n",
                ask_error_string(ask_last_error()));
        return EXIT_FAILURE;
    }

    printf("\nHello, %s!\n", name);
    printf("  age     : %d\n", age);
    printf("  visits  : %ld\n", visits);
    printf("  score   : %.2f\n", score);
    printf("  initial : %c\n", initial);

    free(name);
    return EXIT_SUCCESS;
}
```

A session with an invalid first answer:

```text
$ ./build/basic
Age? Expected a whole number.
Age? 28
Lifetime visits? 1000000
Score? 0.9375
Initial (one character)? R
Name? Ada Lovelace

Hello, Ada Lovelace!
  age     : 28
  visits  : 1000000
  score   : 0.94
  initial : R
```

Press Ctrl+D on Unix to end input. The readers return `false` instead of looping forever; `ask_last_error()` reports `ASK_EOF`.

## Complete API reference

All declarations are in [`src/ask.h`](src/ask.h). `prompt` may be `NULL` to suppress printing. The `out` stream of a `*_from` function may be `NULL` for silent retries.

| Function | Behavior |
| --- | --- |
| `bool ask_int(const char *prompt, int *out)` | Reads a base-10 `int`, retrying until valid or input ends |
| `bool ask_long(const char *prompt, long *out)` | Reads a base-10 `long`, retrying until valid or input ends |
| `bool ask_double(const char *prompt, double *out)` | Reads a finite decimal floating-point value |
| `bool ask_char(const char *prompt, char *out)` | Reads exactly one printable ASCII character |
| `char *ask_line(const char *prompt)` | Reads one line of any length into a caller-owned allocation |
| `ask_error ask_last_error(void)` | Returns the thread-local error code of the last read |
| `const char *ask_error_string(ask_error error)` | Returns a static description; does not change error state |
| `bool ask_int_from(FILE *in, FILE *out, const char *prompt, int *value)` | `ask_int` with caller-supplied streams |
| `bool ask_long_from(FILE *in, FILE *out, const char *prompt, long *value)` | `ask_long` with caller-supplied streams |
| `bool ask_double_from(FILE *in, FILE *out, const char *prompt, double *value)` | `ask_double` with caller-supplied streams |
| `bool ask_char_from(FILE *in, FILE *out, const char *prompt, char *value)` | `ask_char` with caller-supplied streams |
| `char *ask_line_from(FILE *in, FILE *out, const char *prompt)` | `ask_line` with caller-supplied streams |

Every function sets the error state on return. Successful reads set `ASK_OK`.

## Input validation rules

### Integers

`ask_int` and `ask_long` accept an optional sign, base-10 digits, and surrounding whitespace. Values outside the target type's range are rejected.

| Input | `ask_int` | `ask_long` |
| --- | --- | --- |
| `42` | Accept | Accept |
| `  -17 ` | Accept | Accept |
| `+7` | Accept | Accept |
| `3.5`, `12abc`, `0x10` | Reject | Reject |
| Empty line | Reject and retry | Reject and retry |
| `INT_MAX + 1`, `LONG_MIN - 1` | Reject | Reject |

### Floating point

`ask_double` accepts decimal floating-point syntax only:

```text
[whitespace] [sign] ( digits [separator [digits]] | separator digits )
             [ (e|E) [sign] digits ] [whitespace]
```

At least one digit must appear in the significand. Examples accepted in the default `"C"` locale: `3.14`, `-2.5e2`, `1.25E-3`, `.5`, `5.`, `+1e+2`.

Rejected: `0x1p3` and other hexadecimal floats, `nan`, `inf`, `infinity` (including signed and case-variant spellings), trailing junk, malformed exponents, overflow, and underflow. Overflow and underflow are detected through `ERANGE` from `strtod()` and treated as invalid input.

The decimal separator is read from `localeconv()` for the current locale. With the default `"C"` locale it is `.`. The library never changes the locale, and trailing whitespace is handled consistently with leading whitespace.

### Characters

`ask_char` is deliberately ASCII-oriented. It accepts exactly one printable ASCII byte in the range `0x21` to `0x7e`, with optional whitespace on either side. A `char` is one byte, so multi-byte UTF-8 input cannot be represented and is rejected.

| Input | Behavior |
| --- | --- |
| `A`, ` A `, `7`, `!` | Accept |
| `hello`, `AB`, `A B` | Reject and retry |
| Empty line or `   ` | Reject and retry |
| `\xc3\xa9` (UTF-8 "e" with accent) | Reject and retry |
| Non-printable control bytes and DEL | Reject and retry |

Invalid input prints `Expected exactly one printable ASCII character.` and prompts again.

### Lines

`ask_line` accepts any bytes up to the newline. A trailing `\r` is removed for CRLF input. An embedded NUL byte terminates the returned C string, so text after it is not visible to string functions. An empty line is a valid empty string.

## Error handling

Invalid user input is not a fatal error: it triggers another prompt. The error state reports only why a call stopped.

```c
#include "ask.h"
#include <stdio.h>

int main(void)
{
    long count;
    if (!ask_long("Count? ", &count)) {
        ask_error err = ask_last_error();
        /* ASK_EOF    - input ended
         * ASK_NOMEM  - allocation failed
         * ASK_EINVAL - NULL stream or result pointer
         * ASK_EIO    - input stream reported a read error */
        fprintf(stderr, "Could not read count: %s\n",
                ask_error_string(err));
        return 1;
    }
    printf("Count: %ld\n", count);
    return 0;
}
```

`ask_error_string()` handles all defined values and unknown values safely. It does not modify the error state.

At EOF with no characters typed, readers return `false` with `ASK_EOF`. At EOF after some characters, `ask_line` returns those characters as a normal line. If `ferror(in)` is set, no partial line is returned: the buffer is released, `ASK_EIO` is reported, and the stream's error indicator is left set for the caller to clear if desired.

## Custom stream usage

The `*_from` functions accept any readable `FILE *` and any writable `FILE *` (or `NULL` to suppress output).

```c
#include "ask.h"
#include <stdio.h>

int main(void)
{
    FILE *answers = fopen("answers.txt", "r");
    int retries = 0;

    if (answers == NULL) {
        return 1;
    }
    if (ask_int_from(answers, stdout, "Retries? ", &retries)) {
        printf("Got %d\n", retries);
    }
    fclose(answers);
    return 0;
}
```

## Memory ownership rules

- `ask_line` and `ask_line_from` return a `malloc`-allocated string. The caller owns it and must release it with `free()`.
- Typed readers free their internal line buffers before returning, on both success and failure paths.
- If `malloc` or `realloc` fails, the library releases any partial buffer. On a failed `realloc`, the original allocation is freed and the remainder of the line is discarded so a later read starts at the next line.
- Buffer growth doubles capacity with an overflow guard. No memory is retained between calls except the thread-local error code.

## Thread-safety considerations

The error state is thread-local (`_Thread_local` in C11), so concurrent calls do not race on it. Functions use only local buffers and no mutable global data.

This is not complete thread safety. The default functions use the shared `stdin` and `stdout` streams; threads calling them at the same time must coordinate access themselves. The `*_from` variants allow separate streams, but a single `FILE *` must still not be used concurrently without synchronization.

## Build and test instructions

```bash
make test
make clean && make test CC=gcc CFLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -O2'
make clean && make test CC=clang CFLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -O2'
```

The main suite has 101 check sites, including deterministic parser fuzzing, EOF and CRLF cases, strict character validation, rejection of hexadecimal floats and special float values, and POSIX-guarded stream-error tests. A second suite uses injected allocators to verify 24 allocation and failure-path check sites. CI also runs both suites under AddressSanitizer and UndefinedBehaviorSanitizer.

Platform note: the stream-error tests run where POSIX `fileno` and `close` are available; they are skipped elsewhere.

## Project structure

```text
ask/
|-- .github/workflows/ci.yml   # GCC, Clang and sanitizer jobs
|-- docs/                      # SVG banner and logo
|-- examples/basic.c           # runnable example
|-- src/ask.c, src/ask.h       # library implementation and public API
|-- tests/test_ask.c           # functional and parser tests
|-- tests/test_alloc.c         # allocation-failure tests
|-- Makefile                   # lib, example, test, install, clean
`-- README.md
```

## Current roadmap

- [x] Typed readers with retry
- [x] Dynamic line reader
- [x] Injectable streams and thread-local errors
- [x] Stream-error reporting and allocation-failure tests
- [x] GCC and Clang CI with warnings as errors and sanitizers
- [ ] CMake package config
- [ ] CI coverage for additional platforms (not currently tested)
- [ ] `ask_choice()` for menu prompts
- [ ] User-supplied validators and error messages

## Contributing

```bash
git clone https://github.com/roland-yegon/ask.git
cd ask
make test
```

Please add a check to `tests/test_ask.c` for behavior changes and keep the library dependency-free. Bug reports and focused pull requests are welcome.

## License

MIT. See [`LICENSE`](LICENSE).

<div align="center">

<img src="docs/logo.svg" width="48" alt="ask logo">

</div>
