#include "ask.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int age;
    long visits;
    double score;
    char initial;

    if (!ask_int("Age? ", &age)) {
        goto input_ended;
    }
    if (!ask_long("Lifetime visits? ", &visits)) {
        goto input_ended;
    }
    if (!ask_double("Score? ", &score)) {
        goto input_ended;
    }
    if (!ask_char("Initial (one character)? ", &initial)) {
        goto input_ended;
    }

    char *name = ask_line("Name? ");
    if (name == NULL) {
        goto input_ended;
    }

    printf("\nHello, %s!\n", name);
    printf("  age     : %d\n", age);
    printf("  visits  : %ld\n", visits);
    printf("  score   : %.2f\n", score);
    printf("  initial : %c\n", initial);
    free(name);
    return EXIT_SUCCESS;

input_ended:
    fprintf(stderr, "\nCould not read input: %s\n",
            ask_error_string(ask_last_error()));
    return EXIT_FAILURE;
}
