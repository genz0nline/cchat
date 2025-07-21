#include "utils.h"
#include "state.h"

extern struct chat_cfg C;

/*** Only use with clients lock acquired ***/
int get_active_clients_count() {
    int count = 0;

    for (int i = 0; i < C.clients_len; i++) {
        count += C.clients[i]->disconnected == 0;
    }

    return count;
}

/*** Only use with participants lock acquired ***/
int get_participants_count() {
    int count = 0;

    for (int i = 0; i < C.participants_len; i++) {
        count += C.participants[i]->disconnected == 0;
    }

    return count;
}
