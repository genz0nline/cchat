#include "input.h"
#include "log.h"
#include "terminal.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "state.h"

struct chat_cfg C;

void chat_init() {
    C.mode = UNDEFINED;

    C.current_message[0] = '\0';
    C.message[0] = '\0';
    C.nickname_set = 0;
    C.nickname[0] = '\0';
    C.nickname_field[0] = '\0';
    pthread_mutex_init(&C.nickname_mutex, NULL);

    C.clients = NULL;
    C.clients_len = 0;
    C.clients_size = 0;
    C.id_seq = 1;

    C.messages = NULL;
    C.messages_len = 0;
    C.messages_size = 0;
    C.message_offset = 0;
    pthread_mutex_init(&C.message_mutex, NULL);

    C.participants = NULL;
    C.participants_len = 0;
    C.participants_size = 0;
    pthread_mutex_init(&C.participants_mutex, NULL);
}

int main(int argc, char *argv[]) {
    int dev = 0;

    if (argc >= 2 && *argv[1] == 'd') {
        dev = 1;
    }

    chat_init();

    if (log_init(dev)) {
        printf("Couldn't initialize logger\n");
        log_cleanup();
        exit(1);
    }
    atexit(log_cleanup);
    log_print("Logger initialized\n");

    enable_raw_mode();
    log_print("Raw mode enabled\n");

    while (1) {
        refresh_screen();
        process_keypress();
    }

    clean_screen();

    disable_raw_mode();
    log_print("Raw mode disabled\n");

    log_cleanup();
}
