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

void process_field_typing(int key, char *field, char *result, pthread_mutex_t *mutex) {
    int current_len = strlen(field);

    if (key == '\r') {
        if (current_len > 0) {
            pthread_mutex_lock(mutex);
            memcpy(result, field, current_len + 1);
            field[0] = '\0';
            pthread_mutex_unlock(mutex);
        }
    } else if (key == 127) {
        field[current_len - 1] = '\0';
    } else if (current_len >= 1023) {
        return;
    } else {
        field[current_len] = key;
        field[current_len + 1] = '\0';
    }
}

void process_keypress_nickname_negotiation_mode(int key) {
    if (key == '\r' || (32 <= key && key <= 127)) {
        process_field_typing(key, C.nickname_field, C.nickname, &C.nickname_mutex);
    } else {
        switch (key) {
            case CTRL_KEY('q'):
                C.mode = UNDEFINED;
                if (C.mode == CONNECT_NICKNAME_NEGOTIATION) {
                    pthread_cancel(C.connect_thread);
                    pthread_join(C.connect_thread, NULL);
                } else if (C.mode == HOST_NICKNAME_NEGOTIATION) {
                    pthread_cancel(C.accept_thread);
                    pthread_join(C.accept_thread, NULL);
                }
                break;
        }
    }
}

void process_keypress_in_chat(int key) {
    if (key == '\r' || (32 <= key && key <= 127)) {
        process_field_typing(key, C.current_message, C.message, &C.message_mutex);
    } else {
        switch (key) {
            case CTRL_KEY('q'):
                if (C.mode == HOST) {
                    log_print("Canceling accept thread\n");
                    pthread_cancel(C.accept_thread);
                    pthread_join(C.accept_thread, NULL);
                } else if (C.mode == CONNECT) {
                    pthread_cancel(C.connect_thread);
                    pthread_join(C.connect_thread, NULL);
                }
                C.mode = UNDEFINED;
                break;
            case CTRL_KEY('d'):
                C.message_offset -= MESSAGE_BUF_ROWS;
                if (C.message_offset < 0) C.message_offset = 0;
                break;
            case CTRL_KEY('u'):
                C.message_offset += MESSAGE_BUF_ROWS;
                if (C.message_offset > C.messages_len - C.rows + 1) C.message_offset = C.messages_len - C.rows + 1;
                break;
            case CTRL_KEY('j'):
                if (C.message_offset > 0) C.message_offset--;
                break;
            case CTRL_KEY('k'):
                pthread_mutex_lock(&C.message_mutex);
                if (C.message_offset < C.messages_len - C.rows + 1) C.message_offset++;
                pthread_mutex_unlock(&C.message_mutex);
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
            log_print("Processing host keypress\n");
            process_keypress_in_chat(key);
            break;
        case PREPARE_CONNECT:
            process_keypress_prepare_connect_mode(key);
            break;
        case CONNECT_NICKNAME_NEGOTIATION:
            process_keypress_nickname_negotiation_mode(key);
            break;
        case CONNECT:
            log_print("Processing connect keypress\n");
            process_keypress_in_chat(key);
            break;
    }
}
