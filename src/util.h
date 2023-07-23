#ifndef _UTIL_H_
#define _UTIL_H_

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_MSG_LEN  1024
#define MAX_NAME_LEN 20
#define BUFLEN       MAX_MSG_LEN + MAX_NAME_LEN + 4
                     // +4 = '[' before name, "]: " after name

enum parse_string_t { OK, NO_INPUT, END_OF_FILE, TOO_LONG };

/* Functions that should be used in client and server. */
bool parse_int(const char *input, int *i, const char *usage);
int get_string(char *buf, const size_t sz);
/**
 * Determines if the string input represent a valid integer.
 */
bool is_integer(const char *input) {
    int start = 0, len = strlen(input), i;

    if (len >= 1 && input[0] == '-') {
        if (len < 2) {
            return false;
        }
        start = 1;
    }
    for (i = start; i < len; i++) {
        if (!isdigit(input[i])) {
            return false;
        }
    }
    return len > 0;
}

/**
 * Attempts to convert the input string into the integer i.
 * Returns true if the conversion was successful, false otherwise.
 */
bool parse_int(const char *input, int *i, const char *usage) {
    long long long_long_i;

    if (strlen(input) == 0) {
        fprintf(stderr,
                "Error: Invalid input '' received for %s.\n", usage);
        return false;
    }
    if (is_integer(input) && sscanf(input, "%lld", &long_long_i) == 1) {
        *i = (int)long_long_i;
        if (long_long_i != *i) {
            fprintf(stderr, "Error: Integer overflow for %s.\n", usage);
            return false;
        }
    } else {
        fprintf(stderr, "Error: Invalid input '%s' received for %s.\n", input,
                usage);
        return false;
    }
    return true;
}

/**
 * Reads a string fron STDIN up to sz characters. If more than sz characters
 * are supplied, the function consumes them.
 * Returns OK, NO_INPUT, END_OF_FILE, or TOO_LONG.
 */
int get_string(char *buf, const size_t sz) {
    char c;
    int index = 0;
    while(1){
        ssize_t result = read(STDIN_FILENO, &c, 1);
        if(result == -1){
            fprintf(stderr, "Error: read() failed. %s.\n", strerror(errno));
            return NO_INPUT;
        }
        if(result == 0){
            return END_OF_FILE;
        }
        if(index > MAX_MSG_LEN){
            if(c != '\n'){
                char ch;
                while (read(STDIN_FILENO, &ch, 1) > 0 && ch != '\n');
                return TOO_LONG;
            }
            return TOO_LONG;
        }
        buf[index++] = c;
        if(c == '\n'){
            break;
        }
    }
    buf[index-1] = '\0';
    if(strlen(buf) == 0){
        return NO_INPUT;
    }

    return OK;

}

#endif
