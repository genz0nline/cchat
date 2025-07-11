#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>

#include "utils.h"

const char *app_name = "cchat-client";

/*** messaging ***/

void spam(int socket) {
    while (1) {
        sock_send(socket, "Hello, server! I am client!");
        sleep(1);
    }
}

/*** sockets ***/

int sock_init() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) die("socket");
    print_log("socket was created with %d file descriptor\n", fd);
    return fd;
}

void sock_connect(int fd) {
    struct sockaddr_in server_addr = get_localhost_addr(8000);

    socklen_t len = sizeof(server_addr);
    if (connect(fd, (struct sockaddr *)&server_addr, len) == -1) die("connect");
    print_log("socket %d connected to %s\n", fd, inet_ntoa(server_addr.sin_addr));
}

/*** client ***/

void client_connect() {
    int client_socket = sock_init();

    sock_connect(client_socket);

    spam(client_socket);
}

int main(void) {
    client_connect();
}
