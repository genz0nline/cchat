#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    if (log_init()) {
        printf("Couldn't initialize logger\n");
        log_cleanup();
        exit(1);
    }

    log_cleanup();
}
