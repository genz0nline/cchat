#include "input.h"
#include "log.h"
#include "terminal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "state.h"

struct chat_cfg C;

void chat_init() {
    C.mode = UNDEFINED;
    C.clients = NULL;
    char *message = "lorem ipsum";
    memcpy(C.message, message, strlen(message) + 1);
    C.username[0] = '\0';
    C.clients_len = 0;
    C.clients_size = 0;
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
