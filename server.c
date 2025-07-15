#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>

#include <sys/time.h>
#include <pthread.h>

#include "utils.h"

const char *app_name = "cchat-server";

/*** global data ***/

typedef struct SClient {
    pthread_t thread;
    int fd;
    int closed;
} SClient;

typedef struct Chatroom {
    int server_fd;
    pthread_t accept_thread;
    SClient **clients;
    int clients_len;
    int clients_size;
} Chatroom;

Chatroom s_cfg;

void cr_init(void) {
    s_cfg.clients_len = 0;
    s_cfg.clients_size = 0;
    s_cfg.clients = NULL;
}

/*** messaging ***/

void broadcast_message(int client_from, char *message) {
    int client_to;
    
    print_log("broadcasting message from %d\n", client_from);
    for (int i = 0; i < s_cfg.clients_len; i++) {
        client_to = s_cfg.clients[i]->fd;
        if (client_to != client_from && !s_cfg.clients[i]->closed) {
            print_log("\tsending message %s to %d\n", message, client_to);
            sock_send(s_cfg.clients[i]->fd, message);
        }
    }
}

void *handle_client_connection(void *client) {
    SClient *client_ptr = (SClient *)client;

    while (1) {
        char *message = NULL;
        messagelen_t len;

        int status = sock_recv(client_ptr->fd, &message, &len);

        if (status == DISCON) {
            print_log("client %d was disconnected\n", client_ptr->fd);
            client_ptr->closed = 1;
            break;
        } else if (status == IGNORE) {
            print_log("error occured %s\n", strerror(errno));
            continue;
        } else if (status == SUCCESS) {
            broadcast_message(client_ptr->fd, message);
        }

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

void sock_bind() {
    struct sockaddr_in server_addr = get_localhost_addr(8000);

    if (bind(s_cfg.server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
        die("bind");
    print_log("socket %d was bound to %s:%d\n", s_cfg.server_fd, inet_ntoa(server_addr.sin_addr), htons(server_addr.sin_port));
}

void sock_listen() {
    if (listen(s_cfg.server_fd, SOMAXCONN) == -1)
        die("listen");

    print_log("socket %d listens to incomming connections\n", s_cfg.server_fd);
}

void *sock_accept(void *p) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    while (1) {
        int new_socket = accept(s_cfg.server_fd, (struct sockaddr *)&addr, &addr_len);
        if (new_socket == -1) {
            print_log("socket %d failed to accept a connection\n");
            continue;
        }

        if (s_cfg.clients_len == s_cfg.clients_size) {
            s_cfg.clients = (SClient **)realloc(s_cfg.clients, sizeof(SClient *) * (s_cfg.clients_size * 2 + 1));
            s_cfg.clients_size = s_cfg.clients_size * 2 + 1;
        }

        SClient *new_client = (SClient *)malloc(sizeof(SClient));

        new_client->fd = new_socket;
        new_client->closed = 0;

        s_cfg.clients[s_cfg.clients_len] = new_client;

        char *s = inet_ntoa(addr.sin_addr);
        print_log("socket %d accepted connection from %s\n", s_cfg.server_fd, s);

        pthread_create(&new_client->thread, NULL, (void *(*)(void *))handle_client_connection, (void *)s_cfg.clients[s_cfg.clients_len]);

        s_cfg.clients_len++;
    }
}

/*** cleanup ***/

void cleanup() {

    for (int i = 0; i < s_cfg.clients_len; i++) {
        pthread_cancel(s_cfg.clients[i]->thread);
        close(s_cfg.clients[i]->fd);
        free(s_cfg.clients[i]);
    }

    pthread_cancel(s_cfg.accept_thread);

    close(s_cfg.server_fd);
    free(s_cfg.clients);
}

void sig_handler(int sig) {
    cleanup();
    exit(sig);
}

/*** server ***/

void startup_server() {
    cr_init();

    s_cfg.server_fd = sock_init();

    sock_bind();

    sock_listen();

    pthread_create(&s_cfg.accept_thread, NULL, (void *(*)(void *))sock_accept, NULL);

    pthread_join(s_cfg.accept_thread, NULL);
}

/*** main ***/

int main(void) {
    signal(SIGINT, sig_handler);
    signal(SIGSEGV, sig_handler);

    startup_server();
}
