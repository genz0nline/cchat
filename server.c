#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include <pthread.h>

#include "utils.h"

const char *app_name = "cchat-server";

/*** global data ***/

typedef struct Chatroom {
    pthread_t accept_thread;
    pthread_t *clients_threads;
    int *clients_fd;
    int clients_len;
    int clients_size;
} Chatroom;

Chatroom cfg;

void cr_init(void) {
    cfg.clients_len = 0;
    cfg.clients_size = 0;
    cfg.clients_threads = NULL;
    cfg.clients_fd = NULL;
}

/*** messaging ***/

void broadcast_message(int client_from, char *message) {
    int client_to;
    
    for (int i = 0; i < cfg.clients_len; i++) {
        client_to = cfg.clients_fd[i];
        if (client_to != client_from) {
            sock_send(cfg.clients_fd[i], message);
        }
    }
}

void *handle_client_connection(void *client) {
    int client_fd = *(int *)client;

    while (1) {
        char *message = sock_recv(client_fd);
        if (message && strlen(message))
            broadcast_message(client_fd, message);
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

void sock_bind(int fd) {
    struct sockaddr_in server_addr = get_localhost_addr(8000);

    if (bind(fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
        die("bind");
    print_log("socket %d was bound to %s:%d\n", fd, inet_ntoa(server_addr.sin_addr), htons(server_addr.sin_port));
}

void sock_listen(int fd) {
    if (listen(fd, SOMAXCONN) == -1)
        die("listen");

    print_log("socket %d listens to incomming connections\n", fd);
}

void *sock_accept(void *sock_ptr) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int server_socket = *(int *)sock_ptr;

    while (1) {
        int new_socket = accept(server_socket, (struct sockaddr *)&addr, &addr_len);
        if (new_socket == -1) {
            print_log("socket %d failed to accept a connection\n");
            continue;
        }

        if (cfg.clients_len == cfg.clients_size) {
            cfg.clients_threads = (pthread_t *)realloc(cfg.clients_threads, sizeof(pthread_t) * (cfg.clients_size * 2 + 1));
            cfg.clients_fd = (int *)realloc(cfg.clients_fd, sizeof(int) * (cfg.clients_size * 2 + 1));
            cfg.clients_size = cfg.clients_size * 2 + 1;
        }

        cfg.clients_fd[cfg.clients_len] = new_socket;

        char *s = inet_ntoa(addr.sin_addr);
        print_log("socket %d accepted connection from %s\n", server_socket, s);

        pthread_create(&cfg.clients_threads[cfg.clients_len], NULL, (void *(*)(void *))handle_client_connection, (void *)&cfg.clients_fd[cfg.clients_len]);

        cfg.clients_len++;

        print_log("%d/%d\n", cfg.clients_len, cfg.clients_size);
    }
}

/*** server ***/

void startup_server() {
    cr_init();

    int server_socket = sock_init();

    sock_bind(server_socket);

    sock_listen(server_socket);

    pthread_create(&cfg.accept_thread, NULL, (void *(*)(void *))sock_accept, (void *)&server_socket);

    pthread_join(cfg.accept_thread, NULL);
}

/*** main ***/

int main(void) {
    startup_server();
}
