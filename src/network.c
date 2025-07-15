#include <stddef.h>
#include <sys/socket.h>

#include "network.h"
#include "log.h"
#include "state.h"

extern struct chat_cfg C;

void *host_chat(void *p) {
    log_print("Initializing chat server...\n");
    return NULL;
}


void *connect_to_chat(void *p) {
    log_print("Connecting to chat server...\n");
    return NULL;
}
