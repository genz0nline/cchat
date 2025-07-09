#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <stdarg.h>

/*** defines ***/

#define APP_NAME "cchat-server"

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

    fprintf(stderr, "\x1b[33m%s %s |\x1b[39m ", s, APP_NAME);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

/*** sockets ***/

int sock_init() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) die("socket");
    print_log("socket started with %d file descriptor\n", fd);
    return fd;
}

void sock_bind(int fd) {

    char *s_addr = "127.0.0.1";
    int port = 8000;

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    inet_aton(s_addr, &server_addr.sin_addr);
    if (bind(fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
        die("bind");

    print_log("socket %d was bound to %s:%d\n", fd, s_addr, port);
}

void sock_listen(int fd) {
    if (listen(fd, SOMAXCONN) == -1)
        die("listen");

    print_log("socket %d listens to incomming connections\n", fd);
}

void sock_accept(int fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    while (1) {
        int client_fd = accept(fd, (struct sockaddr *)&addr, &addr_len);
        if (client_fd == -1)
            print_log("socket %d failed to accept a connection\n");
        char *s = inet_ntoa(addr.sin_addr);
        print_log("socket %d accepted connection from %s\n", fd, s);
    }
}

/*** server ***/

void startup_server() {
    int server_socket = sock_init();

    sock_bind(server_socket);

    sock_listen(server_socket);

    sock_accept(server_socket);
}

/*** main ***/

int main(void) {

    startup_server();

}
