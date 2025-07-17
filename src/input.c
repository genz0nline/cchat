#include "input.h"
#include "err.h"
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "state.h"
#include "network.h"

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
        case 'q':
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
        case 'q':
            C.mode = UNDEFINED;
            break;
        case '\r':
            C.mode = HOST;
            break;
        default:
            break;
    }
}

void process_keypress_prepare_connect_mode(int key) {
    switch (key) {
        case 'q':
            C.mode = UNDEFINED;
            break;
        case '\r':
            C.mode = CONNECT;
            break;
        default:
            break;
    }
}

void process_keypress_host_mode(int key) {
    switch (key) {
        case 'q':
            C.mode = UNDEFINED;
            break;
        default:
            break;
    }
}

void process_keypress_connect_mode(int key) {
    switch (key) {
        case 'q':
            C.mode = UNDEFINED;
            break;
        default:
            break;
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
        case HOST:
            process_keypress_host_mode(key);
            break;
        case PREPARE_CONNECT:
            process_keypress_prepare_connect_mode(key);
            break;
        case CONNECT:
            process_keypress_connect_mode(key);
            break;
    }
}
