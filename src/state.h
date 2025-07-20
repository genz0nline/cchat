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

typedef struct ChatMessage {
    char *sender_nickname;
    char *content;
} ChatMessage;

struct chat_cfg {
    chat_mode mode;
    int rows;
    int cols;

    char username[32];
    char current_message[1024];
    char message[1024];

    /*** For server mode ***/
    pthread_t accept_thread;
    int server_socket;

    pthread_mutex_t clients_mutex;
    Client **clients;
    size_t clients_len;
    size_t clients_size;

    /*** For client mode ***/
    pthread_t connect_thread;
    int connect_socket;

    /*** Messages ***/
    ChatMessage *messages;
    size_t messages_len;
    size_t messages_size;
};


#endif // STATE_h_
