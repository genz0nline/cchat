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

extern struct chat_cfg C;

uint16_t get_content_length(message_t type) {
    switch (type) {
        case STOC_CLILST: 
            return get_active_clients_count() * (ID_LEN + NN_LEN);
        case STOC_CLICON: 
            return ID_LEN + NN_LEN;
        case STOC_CLIDIS: 
            return ID_LEN;
        case STOC_MSG: 
            return 0; // TODO: 
        case CTOS_MSG: 
            return strlen(C.message);
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

void set_content(char *buf, message_t type, void *p) {
    switch (type) {
        case STOC_CLILST: 
            set_client_list_content(buf);
            break;
        case STOC_CLICON: 
            set_client_connected_content(buf, (Client *)p);
            break;
        case STOC_CLIDIS: 
            set_client_disconnected_content(buf, (Client *)p);
            break;
        case STOC_MSG: 
            break;
        case CTOS_MSG: 
            break;
        case CTOS_CNN: 
            break;
        case STOC_CNNSTAT: 
            break;
        case STOC_CNN: 
            break;
    }
}

char *form_message(message_t type, void *p, size_t *message_len) {
    uint16_t content_length = get_content_length(type);
    *message_len = MD_LEN + content_length;
    if (content_length == 0) die("get_content_length");

    char *buf = malloc(*message_len);

    set_metadata(buf, type, content_length);
    set_content(buf + MD_LEN, type, p);

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

void process_message(message_t type, char *content, uint16_t content_length) {
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
            break;
        case CTOS_MSG: 
            break;
        case CTOS_CNN: 
            break;
        case STOC_CNNSTAT: 
            break;
        case STOC_CNN: 
            break;
    }
}
