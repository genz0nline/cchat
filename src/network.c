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
#include "utils.h"

extern struct chat_cfg C;

int process_protocol_message(int socket, Client *client, uint8_t *status) {
    char metadata[4];
    uint16_t content_length;

    int n;

    if ((n = read_nbytes(socket, metadata, MD_LEN)) == MD_LEN) {
        uint8_t protocol_version = metadata[0];
        if (protocol_version != 0x01) exit(123);

        message_t type = metadata[1];

        memcpy(&content_length, metadata + PV_LEN + MT_LEN, CL_LEN);
        content_length = ntohs(content_length);
        char *content = malloc(content_length);
        
        if ((n = read_nbytes(socket, content, content_length)) == content_length) {
            process_message(type, content, content_length, client, status);
        }
    }
    return n;
}

void *handle_client_connection(void *p) {

    Client *client = (Client *)p;

    char message[1024];
    int n;

    while ((n = process_protocol_message(client->socket, client, NULL)) > 0);

    if (n == 0) {
        client->disconnected = 1;
        server_broadcast_message(STOC_CLIDIS, client, NULL);
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

int negotiate_nickname(Client *client) {
    uint8_t status;

    process_protocol_message(client->socket, client, &status);

    return status;
}

int add_client(int fd) {
    Client *new_client = (Client *)malloc(sizeof(Client));
    new_client->socket = fd;
    new_client->disconnected = 0;
    new_client->id = C.id_seq++;
    sprintf(new_client->nickname, "__new_client_%d", new_client->id);

    int nickname_negotiation_status;
    do {
        nickname_negotiation_status = negotiate_nickname(new_client);
    } while (nickname_negotiation_status != STAT_SUCCESS);

    pthread_mutex_lock(&C.clients_mutex);
    if (C.clients_len >= C.clients_size) {
        C.clients_size = C.clients_size * 2 + 1;
        C.clients = (Client **)realloc(C.clients, (sizeof(Client *) * C.clients_size));
    }

    server_broadcast_message(STOC_CLICON, new_client, NULL);

    C.clients[C.clients_len] = new_client;
    C.clients_len++;

    server_send_message(STOC_CLILST, new_client, NULL);

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

int client_send_message(message_t type, char *content) {
    size_t message_len;
    char *message = form_message(type, NULL, content, &message_len);

    int n = write(C.connect_socket, message, message_len);
    if (n != message_len) return 1;
    return 0;
}

void *handle_sending_messages(void *p) {

    while (1) {
        pthread_mutex_lock(&C.nickname_mutex);
        if (strlen(C.nickname) > 0) {
            if (client_send_message(CTOS_INTRO, C.nickname)) {
                pthread_mutex_unlock(&C.nickname_mutex);
                C.nickname[0] = '\0';
                return NULL;
            }

            C.nickname[0] = '\0';

            uint8_t status;
            process_protocol_message(C.connect_socket, NULL, &status);
            if (status == STAT_SUCCESS) {
                C.nickname_set = 1;
                pthread_mutex_unlock(&C.nickname_mutex);
                C.mode = CONNECT;
                break;
            }
        }
        pthread_mutex_unlock(&C.nickname_mutex);
    }

    while (1) {
        pthread_mutex_lock(&C.message_mutex);
        if (strlen(C.message) > 0) {
            if (client_send_message(CTOS_MSG, C.message)) {
                pthread_mutex_unlock(&C.message_mutex);
                return NULL;
            }
        }
        C.message[0] = '\0';
        pthread_mutex_unlock(&C.message_mutex);
        pthread_testcancel();
    }
    return NULL;
}

void cancel_client_write_thread(void *p) {
    pthread_cancel(*(pthread_t *)p);
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

    pthread_t write_thread;
    pthread_create(&write_thread, NULL, handle_sending_messages, NULL);
    pthread_cleanup_push(cancel_client_write_thread, (void *)&write_thread);

    int nickname_set = 0;
    while (!nickname_set) {
        pthread_mutex_lock(&C.nickname_mutex);
        if (C.nickname_set) nickname_set = 1;
        pthread_mutex_unlock(&C.nickname_mutex);
    }

    while (1) {
        process_protocol_message(C.connect_socket, NULL, NULL);
        pthread_testcancel();
    }

    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    C.mode = UNDEFINED;
    pthread_exit(NULL);
}

