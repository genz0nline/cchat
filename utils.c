#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <stdarg.h>

#include "utils.h"

/*** address ***/

struct sockaddr_in get_localhost_addr(int port) {
    char *s_addr = "127.0.0.1";

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    inet_aton(s_addr, &server_addr.sin_addr);

    return server_addr;
}

/*** error handling ***/

void die(const char *s) {
    perror(s);
    exit(EXIT_FAILURE);
}

/*** time ***/

void get_current_time(char *s) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(s, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d",
                    tm.tm_year + 1900,
                    tm.tm_mon + 1,
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

