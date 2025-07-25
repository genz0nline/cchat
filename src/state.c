#include "state.h"
#include <pthread.h>
#include <stdlib.h>

extern struct chat_cfg C;

void state_init() {
    C.mode = UNDEFINED;

    C.current_message[0] = '\0';
    C.message[0] = '\0';
    pthread_mutex_init(&C.message_mutex, NULL);

    C.nickname_set = 0;
    C.nickname[0] = '\0';
    C.nickname_field[0] = '\0';
    pthread_mutex_init(&C.nickname_mutex, NULL);

    C.clients = NULL;
    C.clients_len = 0;
    C.clients_size = 0;
    C.id_seq = 1;
    pthread_mutex_init(&C.clients_mutex, NULL);

    C.messages = NULL;
    C.messages_len = 0;
    C.messages_size = 0;
    C.message_offset = 0;
    pthread_mutex_init(&C.messages_mutex, NULL);

    C.participants = NULL;
    C.participants_len = 0;
    C.participants_size = 0;
    pthread_mutex_init(&C.participants_mutex, NULL);
}

void state_refresh() {
    // Static data
    C.mode = UNDEFINED;

    C.current_message[0] = '\0';
    C.message[0] = '\0';

    C.nickname_set = 0;
    C.nickname[0] = '\0';
    C.nickname_field[0] = '\0';
    C.id_seq = 1;

    //Dynamic data
    int i;

    if (C.messages_len > 0) {
        for (i = 0; i < C.messages_len; i++) {
            free(C.messages[i].sender_nickname);
            free(C.messages[i].content);
        }
        free(C.messages);

        C.messages = NULL;
        C.messages_len = 0;
        C.messages_size = 0;
    }

    if (C.clients_len > 0) {
        for (i = 0; i < C.clients_len; i++) {
            free(C.clients[i]);
        }
        free(C.clients);

        C.clients = NULL;
        C.clients_len = 0;
        C.clients_size = 0;
    }

    if (C.participants_len > 0) {
        for (i = 0; i < C.participants_len; i++) {
            free(C.participants[i]);
        }
        free(C.participants);

        C.participants = NULL;
        C.participants_len = 0;
        C.participants_size = 0;
    }
}

void state_destroy() {
    state_refresh();
    pthread_mutex_destroy(&C.message_mutex);
    pthread_mutex_destroy(&C.nickname_mutex);
    pthread_mutex_destroy(&C.clients_mutex);
    pthread_mutex_destroy(&C.messages_mutex);
    pthread_mutex_destroy(&C.participants_mutex);
}
