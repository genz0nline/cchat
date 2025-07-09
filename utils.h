#ifndef UTILS_h_
#define UTILS_h_

#include <netinet/in.h>
extern const char *app_name;

void die(const char *s);
void get_current_time(char *s);
void print_log(char *fmt, ...);
struct sockaddr_in get_localhost_addr(int port);

#endif // UTILS_h_
