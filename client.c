#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>

#include "utils.h"

const char *app_name = "cchat-client";

char *message = NULL;

/*** global data ***/

typedef struct Client {
    pthread_t send_thread;
    pthread_t recv_thread;
} Client;

Client cfg;

/*** messaging ***/

void *send_messages(void *socket) {

    int socket_fd = *(int *)socket;

    while (1) {
        if (message) {
            sock_send(socket_fd, message);
            message = NULL;
        }
    }
}

void *gather(void *p) {
    int server_fd = *(int *)p;

    while (1) {
        char *message = sock_recv(server_fd);
        if (message && strlen(message))
            printf("%s", message);
        free(message);
    }
}

/*** sockets ***/

int sock_init() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) die("socket");
    print_log("socket was created with %d file descriptor\n", fd);
    return fd;
}

void sock_connect(int fd) {
    struct sockaddr_in server_addr = get_localhost_addr(8000);

    socklen_t len = sizeof(server_addr);
    if (connect(fd, (struct sockaddr *)&server_addr, len) == -1) die("connect");
    print_log("socket %d connected to %s\n", fd, inet_ntoa(server_addr.sin_addr));
}

/*** input ***/

int get_message(char *buf, size_t buflen) {

    int n = getline(&buf, &buflen, stdin);

    while (n > 0 && strchr("\r\n", buf[n-1])) n--;

    if (n > 0) {
        message = buf;
    }

    return n;
}

/*** client ***/

void client_connect() {
    int connect_socket = sock_init();

    sock_connect(connect_socket);

    pthread_create(&cfg.send_thread, NULL, send_messages, (void *)&connect_socket);
    pthread_create(&cfg.recv_thread, NULL, gather, (void *)&connect_socket);

}

int main(int argc, char *argv[]) {
    char buf[128];

    client_connect();

    while (get_message(buf, 128) >= 0);

    pthread_join(cfg.send_thread, NULL);
    pthread_join(cfg.recv_thread, NULL);
}
