#include "input.h"
#include "log.h"
#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "state.h"

struct chat_cfg C;

void chat_init() {
    C.mode = UNDEFINED;
}

int main(void) {

    chat_init();
    atexit(log_cleanup);

    if (log_init()) {
        printf("Couldn't initialize logger\n");
        log_cleanup();
        exit(1);
    }

    enable_raw_mode();

    while (1) {
        refresh_screen();
        process_keypress();
    }

    clean_screen();

    disable_raw_mode();

    log_cleanup();
}
