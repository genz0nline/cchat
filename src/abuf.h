#ifndef ABUF_h_
#define ABUF_h_

typedef struct abuf {
    char *b;
    int len;
    int size;
} abuf;

abuf ab_init();
void ab_append(abuf *ab, char *s, int len);
void ab_flush(abuf *ab);
void ab_free(abuf *ab);

#endif // ABUF_h_
