#ifndef STATE_h_
#define STATE_h_

#include <pthread.h>

typedef enum {
    UNDEFINED,
    PREPARE_HOST,
    HOST,
    PREPARE_CONNECT,
    CONNECT,
} chat_mode;

typedef struct Client {
    char *nickname;
    pthread_t thread;
    int disconnected;
    int socket;
} Client;

struct chat_cfg {
    chat_mode mode;
    int rows;
    int cols;

    char username[32];

    // For server mode
    pthread_t accept_thread;
    int server_socket;

    pthread_mutex_t clients_mutex;
    Client **clients;
    size_t clients_len;
    size_t clients_size;

    // For client mode
    pthread_t connect_thread;
    int connect_socket;
};

#endif // STATE_h_
