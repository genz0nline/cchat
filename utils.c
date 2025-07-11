#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <stdarg.h>

#include "utils.h"

/*** messaging ***/

void sock_send(int socket, char *message) {
    messagelen_t len = strlen(message);

    unsigned char serialized_len[MESSAGELEN_BUFLEN];
    serialize_len(serialized_len, htons(len));

    write(socket, serialized_len, MESSAGELEN_BUFLEN);
    write(socket, message, len);
}

char *sock_recv(int socket) {
    unsigned char buf[2];
    messagelen_t len;

    read(socket, buf, 2);
    len = ntohs(deserealize_len(buf));

    if (len > 0) {
        char *message = malloc(len);
        read(socket, message, len);
        return message;
    } else {
        return NULL;
    }
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

