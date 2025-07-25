#ifndef STATE_h_
#define STATE_h_

#include <pthread.h>

typedef enum {
    UNDEFINED,
    PREPARE_HOST,
    HOST,
    HOST_NICKNAME_NEGOTIATION,
    PREPARE_CONNECT,
    CONNECT_NICKNAME_NEGOTIATION,
    CONNECT,
} chat_mode;

typedef struct Client {
    int id;
    char nickname[32];
    pthread_t thread;
    int disconnected;
    int socket;
} Client;

typedef struct Participant {
    int id;
    char nickname[32];
    int disconnected;
} Participant;

typedef struct ChatMessage {
    char *sender_nickname;
    char *content;
} ChatMessage;

# define MESSAGE_BUF_ROWS           (C.rows - 1)

struct chat_cfg {
    chat_mode mode;
    int rows;
    int cols;

    int nickname_set;
    pthread_mutex_t nickname_mutex;
    char nickname[32];
    char nickname_field[32];
    char current_message[1024];
    pthread_mutex_t message_mutex;
    char message[1024];

    /*** For server mode ***/
    pthread_t accept_thread;
    int server_socket;

    pthread_mutex_t clients_mutex;
    int id_seq;
    Client **clients;
    size_t clients_len;
    size_t clients_size;

    /*** For client mode ***/
    pthread_t connect_thread;
    int connect_socket;

    /*** Participants ***/
    pthread_mutex_t participants_mutex;
    Participant **participants;
    size_t participants_len;
    size_t participants_size;

    /*** Messages ***/
    pthread_mutex_t messages_mutex;
    ChatMessage *messages;
    size_t messages_len;
    size_t messages_size;
    int message_offset;
};

void state_init();
void state_refresh();
void state_destroy();

#endif // STATE_h_
