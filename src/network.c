#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#include "network.h"
#include "log.h"
#include "state.h"
#include "err.h"

extern struct chat_cfg C;

void *handle_client_connection(void *p) {

    Client *client = (Client *)p;

    char message[1024];
    int n;

    while ((n = read(client->socket, message, 1024)) > 0) {
        printf("%s\n", message);
    }

    if (n == 0) {
        client->disconnected = 1;
        close(client->socket);
        pthread_exit(NULL);
    }

    if (n < 0) {
        die("read");
    }

    return NULL;
}

#define PORT        8000

void close_socket(void *p) {
    int socket = *(int *)p;
    log_print("closing socket %d\n", socket);
    close(socket);
}

void cancel_clients(void *p) {

    log_print("canceling clients_threads\n");

    pthread_mutex_lock(&C.clients_mutex);
    log_print("clients mutex acquired in cancel_clients()\n");

    for (int i = 0; i < C.clients_len; i++) {
        if (!C.clients[i]->disconnected) {
            pthread_cancel(C.clients[i]->thread);
            pthread_join(C.clients[i]->thread, NULL);
            free(C.clients[i]);
        }
    }

    pthread_mutex_unlock(&C.clients_mutex);
    log_print("clients mutex freed in cancel_clients()\n");
}

void cancel_connection(void *p) {
    pthread_cancel(C.connect_thread);
    pthread_join(C.connect_thread, NULL);
}

void free_clients(void *p) {
    if (C.clients)
        free(C.clients);
}

void add_client(int fd) {
    Client *new_client = (Client *)malloc(sizeof(Client));
    new_client->socket = fd;
    new_client->disconnected = 0;

    pthread_mutex_lock(&C.clients_mutex);
    log_print("clients mutex acquired in add_client()\n");
    if (C.clients_len >= C.clients_size) {
        C.clients_size = C.clients_size * 2 + 1;
        C.clients = (Client **)realloc(C.clients, (sizeof(Client *) * C.clients_size));
    }

    C.clients[C.clients_len] = new_client;
    C.clients_len++;
    pthread_mutex_unlock(&C.clients_mutex);
    log_print("clients mutex freed in add_client()\n");

    pthread_create(&new_client->thread, NULL, handle_client_connection, (void *)new_client);
}

void *host_chat(void *p) {
    log_print("Hosting a chat room\n");
    C.server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (C.server_socket == -1) die("socket");
    pthread_cleanup_push(close_socket, (void *)&C.server_socket);
    pthread_cleanup_push(free_clients, NULL);

    log_print("Created accept socket %d\n", C.server_socket);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(C.server_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) die("bind");
    log_print("Bound socket %d to the port %d\n", C.server_socket, PORT);

    if (listen(C.server_socket, SOMAXCONN) == -1) die("listen");
    log_print("Socket %d is accepting connections\n", C.server_socket);

    pthread_mutex_init(&C.clients_mutex, NULL);

    pthread_cleanup_push(cancel_clients, NULL);

    // Connecting host as a client
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) die("socketpair");
    pthread_create(&C.connect_thread, NULL, connect_to_chat, (void *)pair);
    add_client(pair[1]);
    log_print("Accepted loopback connection\n");
    pthread_cleanup_push(cancel_connection, NULL);

    while (1) {
        log_print("In loop\n");
        struct sockaddr_in new_connection;
        socklen_t size = sizeof(new_connection);
        int new_client_fd;
        if ((new_client_fd = accept(C.server_socket, (struct sockaddr *)&new_connection, &size)) == -1) die("accept");

        log_print("Accepted connection from %s\n", inet_ntoa(new_connection.sin_addr));
        add_client(new_client_fd);
    }

    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);

    return NULL;
}

void *connect_to_chat(void *p) {

    int loopback;

    if (p) {
        loopback = 1;
        C.connect_socket = *(int *)p;
    } else {
        loopback = 0;
        C.connect_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (C.connect_socket == -1) die("socket");
    }

    pthread_cleanup_push(close_socket, (void *)&C.connect_socket);

    log_print("Created connect socket %d\n", C.connect_socket);

    if (!loopback) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        inet_aton("127.0.0.1", &addr.sin_addr);

        if (connect(C.connect_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) die("connect");
    }

    while (1) {
        // if (strlen(C.message) > 0) {
        //     if (write(C.connect_socket, C.message, 1024) <= 0) break;
        //     sleep(1);
        // }
        pthread_testcancel();
    }

    pthread_cleanup_pop(1);
    C.mode = UNDEFINED;
    pthread_exit(NULL);
}
