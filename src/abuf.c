#include "abuf.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "err.h"

abuf ab_init() {
    abuf ab;

    ab.b = NULL;
    ab.len = 0;
    ab.size = 0;

    return ab;
}

void ab_append(abuf *ab, char *s, int len) {
    if (ab->len + len > ab->size) {
        ab->b = realloc(ab->b, (ab->len + len) * 2);
        if (ab->b == NULL) die("realloc");

        ab->size = (ab->len + len) * 2;
    }

    memcpy(ab->b + ab->len, s, len);

    ab->len += len;
}

void ab_flush(abuf *ab) {
    write(STDOUT_FILENO, ab->b, ab->len);
    ab->len = 0;
}

void ab_free(abuf *ab) {
    if (ab->b)
        free(ab->b);
}

