#include "utils.h"
#include "proto.h"
#include "state.h"
#include <ctype.h>
#include <errno.h>
#include "log.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

ssize_t read_nbytes(int fd, void *buf, size_t nbytes) {
    size_t total_read = 0;
    char *ptr = (char *)buf;
    while (total_read < nbytes) {
        ssize_t n = read(fd, ptr + total_read, nbytes - total_read);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0) {
            break;
        }
        total_read += n;
    }
    return total_read;
}

/*** Only use with clients lock acquired ***/
int validate_nickname(char *nickname) {
    char *p = nickname;

    int status = STAT_SUCCESS;

    while (*p != '\0' && p - nickname < NN_LEN) {
        if (!isalnum(*p)) {
            status = STAT_INVALID;
            break;
        }
        p++;
    }

    for (int i = 0; i < C.clients_len; i++) {
        Client *client = C.clients[i];
        if (!client->disconnected && strncmp(client->nickname, nickname, NN_LEN) == 0) {
            status = STAT_TAKEN;
            break;
        }
    }

    return status;
}
