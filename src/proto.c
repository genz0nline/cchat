#include "proto.h"
#include "err.h"
#include "state.h"
#include "utils.h"
#include "log.h"
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern struct chat_cfg C;

void server_broadcast_message(message_t type, Client *client, char *content) {
    size_t message_len;
    char *message = form_message(type, client, content, &message_len);
    for (int i = 0; i < C.clients_len; i++) {
        Client *client = C.clients[i];
        if (!client->disconnected)
            write(client->socket, message, message_len);
    }
}

void server_send_message(message_t type, Client* client, char *content) {
    size_t message_len;
    char *message = form_message(type, client, content, &message_len);

    if (!client->disconnected) {
        write(client->socket, message, message_len);
    }
}


uint16_t get_content_length(message_t type, char *content) {
    switch (type) {
        case STOC_CLILST: 
            return get_active_clients_count() * (ID_LEN + NN_LEN);
        case STOC_CLICON: 
            return ID_LEN + NN_LEN;
        case STOC_CLIDIS: 
            return ID_LEN;
        case STOC_MSG: 
            return ID_LEN + strlen((char *)content) + 1;
        case CTOS_MSG: 
            return strlen(C.message) + 1;
        case CTOS_CNN: 
            return NN_LEN;
        case STOC_CNNSTAT: 
            return STAT_LEN;
        case STOC_CNN: 
            return ID_LEN + NN_LEN;
        default:
            return 0;
    }
}

void set_metadata(char *buf, message_t type, uint16_t clen) {
    buf[0] = PROTO_VER;
    buf[1] = type;

    uint16_t n_clen = htons(clen);

    memcpy(buf + PV_LEN + MT_LEN, &n_clen, CL_LEN);
}

void set_client_list_content(char *buf) {
    int offset = 0;

    for (int i = 0; i < C.clients_len; i++) {
        Client *client = C.clients[i];
        uint16_t id = htons(client->id);
        if (!client->disconnected) {
            memcpy(buf + offset, &id, ID_LEN);
            offset += ID_LEN;
            memcpy(buf + offset, client->nickname, NN_LEN);
            offset += NN_LEN;
        }
    }
}

void set_client_connected_content(char *buf, Client *client) {
    uint16_t id = htons(client->id);
    memcpy(buf, &id, ID_LEN); memcpy(buf + ID_LEN, client->nickname, NN_LEN);
}

void set_client_disconnected_content(char *buf, Client *client) {
    uint16_t id = htons(client->id);

    memcpy(buf, &id, ID_LEN);
}

void set_server_to_client_message_content(char *buf, Client *client, char *message) {
    uint16_t id = htons(client->id);
    memcpy(buf, &id, ID_LEN);
    memcpy(buf + ID_LEN, message, strlen(message) + 1);
}

void set_client_to_server_message_content(char *buf, char *message) {
    memcpy(buf, message, strlen(message) + 1);
}

void set_content(char *buf, message_t type, Client *client, char *content) {
    switch (type) {
        case STOC_CLILST: 
            set_client_list_content(buf);
            break;
        case STOC_CLICON: 
            set_client_connected_content(buf, client);
            break;
        case STOC_CLIDIS: 
            set_client_disconnected_content(buf, client);
            break;
        case STOC_MSG: 
            set_server_to_client_message_content(buf, client, content);
            break;
        case CTOS_MSG: 
            set_client_to_server_message_content(buf, content);
            break;
        case CTOS_CNN: 
            break;
        case STOC_CNNSTAT: 
            break;
        case STOC_CNN: 
            break;
    }
}

char *form_message(message_t type, Client *client, char *content, size_t *message_len) {
    uint16_t content_length = get_content_length(type, content);
    *message_len = MD_LEN + content_length;
    if (content_length == 0) die("get_content_length");

    char *buf = malloc(*message_len);

    set_metadata(buf, type, content_length);
    set_content(buf + MD_LEN, type, client, content);

    return buf;
}

void process_client_list_message(char *content, uint16_t content_length) { 
    pthread_mutex_lock(&C.participants_mutex);

    if (C.participants_len) {
        for (int i = 0; i < C.participants_len; i++) {
            free(C.participants[i]);
        }
        C.participants_len = 0;
    }

    int participants_count = content_length / (ID_LEN + NN_LEN);
    C.participants = (Participant **)realloc(C.participants, participants_count * sizeof(Participant *));
    C.participants_size = participants_count;

    int offset = 0;
    for (int i = 0; i < participants_count; i++) {
        C.participants[i] = (Participant *)malloc(sizeof(Participant));
        C.participants[i]->id = ntohs(*(uint16_t *)(content + offset));
        offset += ID_LEN;
        memcpy(C.participants[i]->nickname, content + offset, NN_LEN);
        offset += NN_LEN;
        C.participants[i]->disconnected = 0;
        C.participants_len++;
    }

    pthread_mutex_unlock(&C.participants_mutex);
}

void process_client_connected_message(char *content, uint16_t content_length) {
    pthread_mutex_lock(&C.participants_mutex);
    if (C.participants_len >= C.participants_size) {
        C.participants_size = C.participants_size * 2 + 1;
        C.participants = (Participant **)realloc(C.participants, C.participants_size * sizeof(Participant *));
    }

    Participant *new_participant = (Participant *)malloc(sizeof(Participant));
    new_participant-> id = ntohs(*(uint16_t *)(content));
    memcpy(new_participant->nickname, content + ID_LEN, NN_LEN);
    new_participant->disconnected = 0;

    C.participants[C.participants_len++] = new_participant;
    pthread_mutex_unlock(&C.participants_mutex);
}

void process_client_disconnected_message(char *content, uint16_t content_length) {
    pthread_mutex_lock(&C.participants_mutex);
    uint16_t id = ntohs(*(uint16_t *)(content));
    for (int i = 0; i < C.participants_len; i++) {
        if (C.participants[i]->id == id)
            C.participants[i]->disconnected = 1;
    }
    pthread_mutex_unlock(&C.participants_mutex);
}

void add_message(char *c, char *nickname) {
    pthread_mutex_lock(&C.messages_mutex);
    if (C.messages_len >= C.messages_size) {
        C.messages_size = 2 * C.messages_size + 1;
        C.messages = (ChatMessage *)realloc(C.messages, sizeof(ChatMessage) * C.messages_size);
    }

    ChatMessage new_message;

    new_message.sender_nickname = malloc(16);
    new_message.content = malloc(1024);

    memcpy(new_message.content, c, strlen(c));
    memcpy(new_message.sender_nickname, nickname, strlen(nickname));

    C.messages[C.messages_len++] = new_message;
    pthread_mutex_unlock(&C.messages_mutex);
}

void process_server_to_client_message(char *content, uint16_t content_length) {
    uint16_t id = ntohs(*(uint16_t *)(content));
    pthread_mutex_lock(&C.participants_mutex);
    int i = 0;
    while (i < C.participants_len) {
        if (C.participants[i]->id == id) break;
        i++;
    }
    if (i == C.participants_len) {
        pthread_mutex_unlock(&C.participants_mutex);
        return;
    }
    char *message = malloc(content_length + 1);
    memcpy(message, content + ID_LEN, content_length - ID_LEN);
    message[content_length - ID_LEN] = '\0';
    add_message(message, C.participants[i]->nickname);
    free(message);
    pthread_mutex_unlock(&C.participants_mutex);
}


void process_client_to_server_message(char *content, uint16_t content_length, Client *client) {
    pthread_mutex_lock(&C.clients_mutex);
    log_print("Broadcasting message\n");
    server_broadcast_message(STOC_MSG, client, content);
    pthread_mutex_unlock(&C.clients_mutex);
}

void process_message(message_t type, char *content, uint16_t content_length, Client *client) {
    switch (type) {
        case STOC_CLILST: 
            process_client_list_message(content, content_length);
            break;
        case STOC_CLICON: 
            process_client_connected_message(content, content_length);
            break;
        case STOC_CLIDIS: 
            process_client_disconnected_message(content, content_length);
            break;
        case STOC_MSG: 
            process_server_to_client_message(content, content_length);
            break;
        case CTOS_MSG: 
            process_client_to_server_message(content, content_length, client);
            break;
        case CTOS_CNN: 
            break;
        case STOC_CNNSTAT: 
            break;
        case STOC_CNN: 
            break;
    }
}
