#include <pthread.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

#include "network.h"
#include "log.h"
#include "proto.h"
#include "state.h"
#include "err.h"

extern struct chat_cfg C;

void broadcast_message(message_t type, void *p) {
    size_t message_len;
    char *message = form_message(type, p, &message_len);
    for (int i = 0; i < C.clients_len; i++) {
        Client *client = C.clients[i];
        if (!client->disconnected)
            write(client->socket, message, message_len);
    }
}

void send_message(message_t type, Client* to, void *p) {
    size_t message_len;
    char *message = form_message(type, p, &message_len);

    if (!to->disconnected) {
        write(to->socket, message, message_len);
    }
}

void add_message(char *c, int client) {
    pthread_mutex_lock(&C.messages_mutex);
    if (C.messages_len >= C.messages_size) {
        C.messages_size = 2 * C.messages_size + 1;
        C.messages = (ChatMessage *)realloc(C.messages, sizeof(ChatMessage) * C.messages_size);
    }

    ChatMessage new_message;

    new_message.sender_nickname = malloc(16);
    new_message.content = malloc(1024);

    memcpy(new_message.content, c, strlen(c));
    sprintf(new_message.sender_nickname, "%d", client);

    C.messages[C.messages_len++] = new_message;
    pthread_mutex_unlock(&C.messages_mutex);
}

void *handle_client_connection(void *p) {

    Client *client = (Client *)p;

    char message[1024];
    int n;

    while ((n = read(client->socket, message, 1024)) > 0) {
        add_message(message, client->id);
    }

    if (n == 0) {
        client->disconnected = 1;
        broadcast_message(STOC_CLIDIS, client);
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
    log_print("Closing socket %d\n", socket);
    close(socket);
}

void cancel_clients(void *p) {
    log_print("Canceling clients_threads\n");

    pthread_mutex_lock(&C.clients_mutex);
    for (int i = 0; i < C.clients_len; i++) {
        if (!C.clients[i]->disconnected) {
            pthread_cancel(C.clients[i]->thread);
            pthread_join(C.clients[i]->thread, NULL);
            free(C.clients[i]);
        }
    }
    pthread_mutex_unlock(&C.clients_mutex);
}

void cancel_connection(void *p) {
    pthread_cancel(C.connect_thread);
    pthread_join(C.connect_thread, NULL);
}

void free_clients(void *p) {
    if (C.clients)
        free(C.clients);
}

int add_client(int fd) {
    Client *new_client = (Client *)malloc(sizeof(Client));
    new_client->socket = fd;
    new_client->disconnected = 0;
    new_client->id = C.id_seq++;
    sprintf(new_client->nickname, "__new_client_%d", new_client->id);

    pthread_mutex_lock(&C.clients_mutex);
    if (C.clients_len >= C.clients_size) {
        C.clients_size = C.clients_size * 2 + 1;
        C.clients = (Client **)realloc(C.clients, (sizeof(Client *) * C.clients_size));
    }

    broadcast_message(STOC_CLICON, new_client);

    C.clients[C.clients_len] = new_client;
    C.clients_len++;

    send_message(STOC_CLILST, new_client, NULL);

    pthread_mutex_unlock(&C.clients_mutex);

    pthread_create(&new_client->thread, NULL, handle_client_connection, (void *)new_client);

    log_print("Added client %d\n", fd);
    return new_client->id;
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
    pthread_mutex_init(&C.messages_mutex, NULL);
    pthread_cleanup_push((void (*)(void *))pthread_mutex_destroy, &C.clients_mutex);
    pthread_cleanup_push((void (*)(void *))pthread_mutex_destroy, &C.messages_mutex);

    pthread_cleanup_push(cancel_clients, NULL);

    // Connecting host as a client
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) die("socketpair");
    pthread_create(&C.connect_thread, NULL, connect_to_chat, (void *)pair);
    int id = add_client(pair[1]);
    log_print("Accepted loopback connection\n");
    pthread_cleanup_push(cancel_connection, NULL);

    while (1) {
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

    // while (1) {
    //     if (strlen(C.message) > 0) {
    //         if (write(C.connect_socket, C.message, 1024) <= 0) {
    //             break;
    //         }
    //         C.message[0] = '\0';
    //     }
    //     pthread_testcancel();
    // }

    while (1) {

        char metadata[4];
        uint16_t content_length;


        if (read(C.connect_socket, metadata, MD_LEN)) {
            log_print("Read metadata\n");
            uint8_t protocol_version = metadata[0];
            assert(protocol_version == 1); // TODO: It has to be something else

            message_t type = metadata[1];

            memcpy(&content_length, metadata + PV_LEN + MT_LEN, CL_LEN);
            content_length = ntohs(content_length);
            char *content = malloc(content_length);

            if (read(C.connect_socket, content, content_length) == content_length) {
                log_print("Read message of type %d\n", type);
                process_message(type, content, content_length);
            }
        }

        pthread_testcancel();
    }

    pthread_cleanup_pop(1);
    C.mode = UNDEFINED;
    pthread_exit(NULL);
}
