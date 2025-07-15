#ifndef STATE_h_
#define STATE_h_

#include <pthread.h>

typedef enum {
    UNDEFINED,
    HOST,
    CONNECT,
} chat_mode;

struct chat_cfg {
    chat_mode mode;
    int rows;
    int cols;

    // For server mode
    pthread_t accept_thread;
    int server_socket;
    pthread_t *clients_threads;
    int *clients_socktes;

    // For client mode
    pthread_t connect_thread;
    int connect_socket;
};

#endif // STATE_h_
