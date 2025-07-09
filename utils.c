#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#include "utils.h"

/*** error handling ***/

void die(const char *s) {
    perror(s);
    exit(EXIT_FAILURE);
}

/*** time ***/

void get_current_time(char *s) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    snprintf(s, 20, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d",
                    tm.tm_year,
                    tm.tm_mon,
                    tm.tm_mday,
                    tm.tm_hour,
                    tm.tm_min,
                    tm.tm_sec);

}

/*** logging ***/

void print_log(char *fmt, ...) {
    char s[20];
    get_current_time(s);

    fprintf(stderr, "\x1b[33m%s %s |\x1b[39m ", s, app_name);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

