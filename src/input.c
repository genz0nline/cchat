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

void process_keypress() {
    char *buf;

    int key = get_key();
    if (key == -1) return;

    if (C.mode == UNDEFINED) {
        switch (key) {
            case 'q':
                exit(0);
            case 'h':
                pthread_create(&C.accept_thread, NULL, host_chat, NULL);
                break;
            case 'c':
                pthread_create(&C.accept_thread, NULL, connect_to_chat, NULL);
                C.mode = CONNECT;
                break;
        }
    } else {
        switch (key) {
            case 'q':
                C.mode = UNDEFINED;
        }
    }
}
