#ifndef UTILS_h_
#define UTILS_h_

#include <stdint.h>
#include <netinet/in.h>
extern const char *app_name;

/*** defines ***/

typedef uint16_t messagelen_t;
#define MESSAGELEN_BUFLEN   2

void die(const char *s);
void get_current_time(char *s, char delim);
void print_log(char *fmt, ...);
struct sockaddr_in get_localhost_addr(int port);

void serialize_len(unsigned char buf[MESSAGELEN_BUFLEN], messagelen_t num);
messagelen_t deserealize_len(unsigned char *buf);

enum status {
    SUCCESS = 0,
    DISCON, // disconnected
    IGNORE, // anything else, can be ignored for now
};

int sock_send(int socket, char *message);
int sock_recv(int socket, char **message, messagelen_t *len);

#endif // UTILS_h_
