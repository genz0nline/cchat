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

typedef struct CClient {
    int connect_socket;
    pthread_t send_thread;
    pthread_t recv_thread;
} CClient;

CClient c_cfg;

/*** messaging ***/

void *send_messages(void *p) {
    while (1) {
        if (message) {
            sock_send(c_cfg.connect_socket, message);
            message = NULL;
        }
    }

    return NULL;
}

void *gather(void *p) {
    while (1) {
        char *message = NULL;
        messagelen_t len;

        int status = sock_recv(c_cfg.connect_socket, &message, &len);

        if (status == IGNORE) continue;
        if (status == DISCON) break;
        printf("%s", message);

        free(message);
    }

    return NULL;
}

/*** sockets ***/

int sock_init() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) die("socket");
    print_log("socket was created with %d file descriptor\n", fd);
    return fd;
}

void sock_connect() {
    struct sockaddr_in server_addr = get_localhost_addr(8000);

    socklen_t len = sizeof(server_addr);
    if (connect(c_cfg.connect_socket, (struct sockaddr *)&server_addr, len) == -1) die("connect");
    print_log("socket %d connected to %s\n", c_cfg.connect_socket, inet_ntoa(server_addr.sin_addr));
}

/*** input ***/

int get_message(char *buf, size_t buflen) {

    int n = getline(&buf, &buflen, stdin);

    if (n > 0) {
        message = buf;
    }

    return n;
}

/*** client ***/

void client_connect() {
    c_cfg.connect_socket = sock_init();

    sock_connect();

    pthread_create(&c_cfg.send_thread, NULL, send_messages, NULL);
    pthread_create(&c_cfg.recv_thread, NULL, gather, NULL);

}

int main(int argc, char *argv[]) {
    char buf[128];

    client_connect();

    while (get_message(buf, 128));

    close(c_cfg.connect_socket);
    pthread_join(c_cfg.send_thread, NULL);
    pthread_join(c_cfg.recv_thread, NULL);
}
