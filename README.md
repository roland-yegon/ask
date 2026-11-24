<div align="center">

<img src="docs/banner.svg" alt="ask — validated user input for C" width="720">

<br>

<a href="https://github.com/roland-yegon/ask/actions/workflows/ci.yml"><img src="https://github.com/roland-yegon/ask/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
<a href="https://en.wikipedia.org/wiki/C11_(C_standard_revision)"><img src="https://img.shields.io/badge/C-C11-00599C.svg?logo=c&logoColor=white" alt="C11"></a>
<img src="https://img.shields.io/badge/dependencies-zero-brightgreen.svg" alt="Zero dependencies">
<a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-green.svg" alt="MIT license"></a>
<a href="https://github.com/roland-yegon/ask/pulls"><img src="https://img.shields.io/badge/PRs-welcome-ff69b4.svg" alt="PRs welcome"></a>

### Prompt. Validate. Repeat.

**`ask` is a tiny C library that keeps asking until the user gives you a valid answer.**
No `scanf` cleanup dances, no hand-rolled retry loops, no fixed-size buffers.

<a href="#quick-start">Quick start</a> &nbsp;·&nbsp; <a href="#api-reference">API reference</a> &nbsp;·&nbsp; <a href="#build--test">Build &amp; test</a> &nbsp;·&nbsp; <a href="#faq">FAQ</a>

</div>

---

## Why `ask`?

Reading a number from a human in C usually looks like this:

```c
int age;
int rc;
do {
    printf("Age? ");
    rc = scanf("%d", &age);
    if (rc != 1) {
        puts("Expected a whole number.");
        while (getchar() != '\n' && !feof(stdin)) {
            /* discard the junk so the next scan isn't poisoned */
        }
    }
} while (rc != 1);
```

With `ask`, that becomes one line:

```c
#include "ask.h"

int age;
ask_int("Age? ", &age);   /* retries until valid, or until EOF */
```

> [!TIP]
> Every reader consumes a **whole line**, so a bad answer can never leak into the next prompt.

---

## Features

| | Feature | What you get |
|:--|:--|:--|
| 🎯 | **Retry built in** | Invalid input is rejected with a helpful message and the prompt is repeated |
| 🔢 | **Four typed readers** | `int`, `long`, `double`, and `char`, with range and syntax validation |
| 📏 | **Unbounded lines** | `ask_line` grows its buffer as the user types — no truncated names |
| 🔌 | **Injectable streams** | Every function has a `*_from(FILE *, FILE *, ...)` variant for files, sockets, or tests |
| 🧵 | **Thread-local errors** | `ask_last_error()` distinguishes EOF, OOM, and bad arguments safely |
| 🪶 | **Zero dependencies** | Plain C11 and libc; a single static library, no build-time generators |
| ✅ | **CI on GCC + Clang** | Warnings are errors; the test suite runs on every push |

---

## Install

```bash
git clone https://github.com/roland-yegon/ask.git
cd ask
make                       # builds build/libask.a and build/basic
sudo make install          # optional: /usr/local/include + /usr/local/lib
```

Then compile against it:

```bash
cc -std=c11 app.c -lask -o app
# or, without installing:
cc -std=c11 -Iask/src app.c ask/build/libask.a -o app
```

---

## Quick start

```c
#include "ask.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int age;
    double score;
    char initial;

    if (!ask_int("Age? ", &age) ||
        !ask_double("Score? ", &score) ||
        !ask_char("Initial? ", &initial)) {
        fprintf(stderr, "Input ended: %s\n",
                ask_error_string(ask_last_error()));
        return 1;
    }

    char *name = ask_line("Name? ");
    if (name == NULL) {
        fprintf(stderr, "Input ended: %s\n",
                ask_error_string(ask_last_error()));
        return 1;
    }

    printf("\nHello, %s! %d · %.2f · %c\n", name, age, score, initial);
    free(name);             /* ask_line always returns heap memory */
    return 0;
}
```

A real session, including a deliberately bad first answer:

```text
$ ./build/basic
Age? Expected a whole number.
Age? 28
Lifetime visits? 1000000
Score? 0.9375
Initial? R
Name? Ada Lovelace

Hello, Ada Lovelace!
  age     : 28
  visits  : 1000000
  score   : 0.94
  initial : R
```

<details>
<summary><b>What happens at EOF?</b></summary>

Press <kbd>Ctrl</kbd> + <kbd>D</kbd> on Unix, or <kbd>Ctrl</kbd> + <kbd>Z</kbd> on Windows. The reader returns `false` immediately — it will not spin forever on a closed stream. `ask_last_error()` then reports `ASK_EOF`.

</details>

---

## API reference

All functions live in [`src/ask.h`](src/ask.h).

| Function | Reads | Notes |
|:--|:--|:--|
| `bool ask_int(const char *prompt, int *out)` | whole number | Retries; rejects overflow and trailing junk |
| `bool ask_long(const char *prompt, long *out)` | whole number | Full `long` range |
| `bool ask_double(const char *prompt, double *out)` | floating point | Scientific notation OK; `nan`, `inf`, overflow, and underflow rejected |
| `bool ask_char(const char *prompt, char *out)` | character | First non-whitespace character of the line; the rest is ignored |
| `char *ask_line(const char *prompt)` | one whole line | Dynamically sized; caller must `free()`; `NULL` on EOF with nothing typed |
| `ask_error ask_last_error(void)` | — | Thread-local result of the last call |
| `const char *ask_error_string(ask_error e)` | — | `"success"`, `"end of input"`, `"out of memory"`, `"invalid argument"` |
| `bool ask_*_from(FILE *in, FILE *out, ...)` | as above | Same behavior with caller-supplied streams |

`prompt` may be `NULL` to print nothing. Surrounding whitespace is ignored for all typed values.

### Validation rules

| Input | `ask_int` | `ask_double` | `ask_char` |
|:--|:--|:--|:--|
| `42` / `  -17 ` | ✅ | ✅ | `4` |
| `3.5` / `-2.5e2` | ❌ | ✅ | `3` |
| `12abc` | ❌ | ❌ | `1` |
| `nan` / `inf` / `1e999` | ❌ | ❌ | `n` / `i` / `1` |
| blank line | ❌ | ❌ | ❌ (retries) |
| `2147483648` (on 32-bit `int`) | ❌ overflow | ✅ | `2` |

`ask_line` accepts any text, strips a trailing `\r` (CRLF input), and returns a C string — an embedded NUL byte therefore terminates the visible text.

### Error handling

```c
long count;
if (!ask_long("Count? ", &count)) {
    ask_error err = ask_last_error();
    /* ASK_EOF    → user pressed Ctrl+D
     * ASK_NOMEM  → allocation failed
     * ASK_EINVAL → NULL stream or NULL output pointer */
    fprintf(stderr, "Could not read count: %s\n", ask_error_string(err));
    return 1;
}
```

### Custom streams

The `*_from` variants make the library usable with redirected input, files, sockets, or test doubles:

```c
FILE *answers = fopen("answers.txt", "r");
int retries = 0;

if (answers != NULL &&
    ask_int_from(answers, stdout, "Retries? ", &retries)) {
    printf("Got %d\n", retries);
}
fclose(answers);
```

---

## Build & test

```bash
make                        # static library + example
make test                   # 60+ dependency-free assertions
make clean                  # remove build/
make install PREFIX=~/.local
```

Run the suite against both compilers with warnings promoted to errors — exactly what CI does:

```bash
make clean && make test CC=gcc CFLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -O2'
make clean && make test CC=clang CFLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -O2'
```

The tests cover valid and invalid values, retry transcripts, overflow, EOF, CRLF, blank lines, buffer growth past the initial capacity, and error reporting.

---

## Project structure

```text
ask/
├── .github/workflows/ci.yml   # GCC + Clang build and test matrix
├── docs/
│   ├── banner.svg             # this README's hero
│   └── logo.svg               # square mark
├── examples/basic.c           # runnable tour of all five readers
├── src/
│   ├── ask.h                  # public API, fully documented
│   └── ask.c                  # implementation (~300 LOC)
├── tests/test_ask.c           # zero-dependency test suite
├── Makefile                   # lib, example, test, install, clean
└── README.md
```

---

## Roadmap

- [x] Typed readers with automatic retry
- [x] Dynamically growing line reader
- [x] Injectable streams and thread-local error reporting
- [x] GCC + Clang CI with `-Werror`
- [ ] CMake package config for `find_package(ask)`
- [ ] Windows / MSVC CI
- [ ] `ask_choice()` for menu prompts
- [ ] User-supplied validators and custom error messages

---

## FAQ

<details>
<summary><b>Why not just use <code>scanf</code>?</b></summary>

`scanf` leaves unmatched input in the stream, so a failed conversion poisons every later read unless you manually drain `stdin`. `ask` reads a full line per attempt, validates it, and either returns a clean value or asks again.

</details>

<details>
<summary><b>Can it handle very long input?</b></summary>

Yes. `ask_line` starts with a small buffer and doubles it until the whole line fits. If allocation fails, it frees the partial result and discards the rest of the line so the next read starts cleanly. `ask_last_error()` reports `ASK_NOMEM`.

</details>

<details>
<summary><b>Is it thread-safe?</b></summary>

Each call uses only local buffers, and error state is thread-local (C11 `_Thread_local`). All the high-level functions still share `stdin`/`stdout`, so coordinate interactive prompts between threads; use the `*_from` variants with separate streams for true parallelism.

</details>

<details>
<summary><b>Does it work in C++?</b></summary>

`ask.h` wraps its declarations in `extern "C"`, so you can include it from C++ and link the C library normally.

</details>

---

## Contributing

```bash
git clone https://github.com/roland-yegon/ask.git
cd ask
make test
# make your change, add a check to tests/test_ask.c, open a PR
```

Bug reports, thoughts on validation semantics, and small PRs are all welcome.

---

<div align="center">

<img src="docs/logo.svg" width="48" alt="ask logo">

<sub>MIT licensed · Built with C11 and a healthy dislike of <code>scanf</code> loops.</sub>

</div>
