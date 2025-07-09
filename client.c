#include <arpa/inet.h>
#include <sys/socket.h>

#include "utils.h"

const char *app_name = "cchat-client";

/*** sockets ***/

int sock_init() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) die("socket");
    print_log("socket was created with %d file descriptor\n", fd);
    return fd;
}

void sock_connect(int fd) {
    char *s_addr = "127.0.0.1";
    int port = 8000;

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    inet_aton(s_addr, &server_addr.sin_addr);

    socklen_t len = sizeof(server_addr);

    if (connect(fd, (struct sockaddr *)&server_addr, len) == -1) die("connect");
    print_log("socket %d connected to %s\n", fd, inet_ntoa(server_addr.sin_addr));
}

/*** client ***/

void client_connect() {
    int client_socket = sock_init();

    sock_connect(client_socket);
}

int main(void) {
    client_connect();
}
