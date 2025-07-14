#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <stdarg.h>

#include "utils.h"

/*** messaging ***/

int sock_send(int socket, char *message) {
    messagelen_t len = strlen(message) + 1;

    unsigned char serialized_len[MESSAGELEN_BUFLEN];
    serialize_len(serialized_len, htons(len));

    print_log("len = %u, serialized_len = %u\n", len, serialized_len);

    int result = send(socket, serialized_len, MESSAGELEN_BUFLEN, 0);

    if (result < 0) {
        if (errno == EPIPE) return DISCON;
        return IGNORE;
    }

    result = send(socket, message, len, 0);

    if (result < 0) {
        if (errno == EPIPE) return DISCON;
        return IGNORE;
    }

    print_log("sent %d bytes\n", result);

    return SUCCESS;
}

int sock_recv(int socket, char **message, messagelen_t *len) {
    unsigned char buf[2];

    int n = recv(socket, buf, MESSAGELEN_BUFLEN, 0);

    if (n < 0) {
        return IGNORE;
    } else if (n == 0) {
        return DISCON;
    }

    *len = ntohs(deserealize_len(buf));

    *message = realloc(*message, *len);
    n = recv(socket, *message, *len, 0);
    if (n < 0) {
        return IGNORE;
    } else if (n == 0) {
        return DISCON;
    } else if (n < *len) {
        *len = n;
    }

    print_log("received %d bytes\n", *len);

    return SUCCESS;
}

/*** serialization ***/

void serialize_len(unsigned char buf[MESSAGELEN_BUFLEN], messagelen_t num) {
    for (int i = 0; i < MESSAGELEN_BUFLEN; i++) {
        buf[i] = ((unsigned char *)&num)[i];
    }
}

messagelen_t deserealize_len(unsigned char *buf) {
    return *(uint16_t *)(buf);
}

/*** address ***/

struct sockaddr_in get_localhost_addr(int port) {
    char *s_addr = "127.0.0.1";

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    inet_aton(s_addr, &server_addr.sin_addr);

    return server_addr;
}

/*** error handling ***/

void die(const char *s) {
    print_log("death - ");
    perror(s);
    exit(EXIT_FAILURE);
}

/*** time ***/

void get_current_time(char *s, char delim) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(s, "%.4d-%.2d-%.2d%c%.2d:%.2d:%.2d",
                    tm.tm_year + 1900,
                    tm.tm_mon + 1,
                    tm.tm_mday,
                    delim,
                    tm.tm_hour,
                    tm.tm_min,
                    tm.tm_sec);

}

/*** logging ***/

void print_log(char *fmt, ...) {
    char time[20];
    get_current_time(time, ' ');

    fprintf(stderr, "\x1b[33m%s %s |\x1b[39m ", time, app_name);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

