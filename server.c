#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>

#include "utils.h"

const char *app_name = "cchat-server";

/*** sockets ***/

int sock_init() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) die("socket");
    print_log("socket was created with %d file descriptor\n", fd);
    return fd;
}

void sock_bind(int fd) {
    struct sockaddr_in server_addr = get_localhost_addr(8000);

    if (bind(fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
        die("bind");
    print_log("socket %d was bound to %s:%d\n", fd, inet_ntoa(server_addr.sin_addr), htons(server_addr.sin_port));
}

void sock_listen(int fd) {
    if (listen(fd, SOMAXCONN) == -1)
        die("listen");

    print_log("socket %d listens to incomming connections\n", fd);
}

void sock_accept(int fd) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    while (1) {
        int client_fd = accept(fd, (struct sockaddr *)&addr, &addr_len);
        if (client_fd == -1)
            print_log("socket %d failed to accept a connection\n");
        char *s = inet_ntoa(addr.sin_addr);
        print_log("socket %d accepted connection from %s\n", fd, s);
    }
}

/*** server ***/

void startup_server() {
    int server_socket = sock_init();

    sock_bind(server_socket);

    sock_listen(server_socket);

    sock_accept(server_socket);
}

/*** main ***/

int main(void) {
    startup_server();
}
