#include "input.h"
#include "err.h"
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "log.h"
#include <errno.h>
#include "network.h"
#include "proto.h"
#include "state.h"

#define CTRL_KEY(k) ((k) & 0x1f)

enum keys {
    ESC_KEY=1000,
};

extern struct chat_cfg C;

int get_key() {
    int nread;
    char c;
    nread = read(STDIN_FILENO, &c, 1);
    if (nread != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
        else return -1;
    }

    if (c == '\x1b') {
        return ESC_KEY; // just for now, we'll think about it later
    } else {
        return c;
    }
}

void process_keypress_undefined_mode(int key) {
    switch (key) {
        case CTRL_KEY('q'):
            exit(0);
        case 'h':
            C.mode = PREPARE_HOST;
            break;
        case 'c':
            C.mode = PREPARE_CONNECT;
            break;
        default:
            break;
    }
}

void process_keypress_prepare_host_mode(int key) {
    switch (key) {
        case CTRL_KEY('q'):
            C.mode = UNDEFINED;
            break;
        case '\r':
            pthread_create(&C.accept_thread, NULL, host_chat, NULL);
            C.mode = HOST_NICKNAME_NEGOTIATION;
            break;
        default:
            break;
    }
}

void process_keypress_prepare_connect_mode(int key) {
    switch (key) {
        case CTRL_KEY('q'):
            C.mode = UNDEFINED;
            break;
        case '\r':
            C.mode = CONNECT_NICKNAME_NEGOTIATION;
            pthread_create(&C.connect_thread, NULL, connect_to_chat, NULL);
            break;
        default:
            break;
    }
}

void process_nickname_typing(int key) {
    int current_message_len = strlen(C.nickname_field);

    if (key == '\r') {
        if (current_message_len > 0) {
            pthread_mutex_lock(&C.nickname_mutex);
            memcpy(C.nickname, C.nickname_field, NN_LEN);
            C.nickname_field[0] = '\0';
            pthread_mutex_unlock(&C.nickname_mutex);
        }
    } else if (key == 127) {
        C.nickname_field[current_message_len - 1] = '\0';
    } else if (current_message_len >= NN_LEN - 1) {
        return;
    } else {
        C.nickname_field[current_message_len] = key;
        C.nickname_field[current_message_len + 1] = '\0';
    }
}

void process_keypress_nickname_negotiation_mode(int key) {
    if (key == '\r' || (32 <= key && key <= 127)) {
        process_nickname_typing(key);
    } else {
        switch (key) {
            case CTRL_KEY('q'):
                C.mode = UNDEFINED;
                break;
        }
    }
}

void process_message_typing(int key) {
    int current_message_len = strlen(C.current_message);

    if (key == '\r') {
        if (current_message_len > 0) {
            pthread_mutex_lock(&C.message_mutex);
            memcpy(C.message, C.current_message, current_message_len + 1);
            C.current_message[0] = '\0';
            pthread_mutex_unlock(&C.message_mutex);
        }
    } else if (key == 127) {
        C.current_message[current_message_len - 1] = '\0';
    } else if (current_message_len >= 1023) {
        return;
    } else {
        C.current_message[current_message_len] = key;
        C.current_message[current_message_len + 1] = '\0';
    }
}

void process_keypress_host_mode(int key) {
    if (key == '\r' || (32 <= key && key <= 127)) {
        process_message_typing(key);
    } else {
        switch (key) {
            case CTRL_KEY('q'):
                pthread_cancel(C.accept_thread);
                pthread_join(C.accept_thread, NULL);
                C.mode = UNDEFINED;
                break;
            default:
                break;
        }
    }
}

void process_keypress_connect_mode(int key) {
    if (key == '\r' || (32 <= key && key <= 127)) {
        process_message_typing(key);
    } else {
        switch (key) {
            case CTRL_KEY('q'):
                pthread_cancel(C.connect_thread);
                pthread_join(C.connect_thread, NULL);
                C.mode = UNDEFINED;
                break;
            default:
                break;
        }
    }
}

void process_keypress() {
    char *buf;

    int key = get_key();
    if (key == -1) return;

    switch (C.mode) {
        case UNDEFINED:
            process_keypress_undefined_mode(key);
            break;
        case PREPARE_HOST:
            process_keypress_prepare_host_mode(key);
            break;
        case HOST_NICKNAME_NEGOTIATION:
            process_keypress_nickname_negotiation_mode(key);
            break;
        case HOST:
            process_keypress_host_mode(key);
            break;
        case PREPARE_CONNECT:
            process_keypress_prepare_connect_mode(key);
            break;
        case CONNECT_NICKNAME_NEGOTIATION:
            process_keypress_nickname_negotiation_mode(key);
            break;
        case CONNECT:
            process_keypress_connect_mode(key);
            break;
    }
}
