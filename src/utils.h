#ifndef UTILS_h_
#define UTILS_h_

#include <sys/types.h>

int get_active_clients_count();
int get_participants_count();
ssize_t read_nbytes(int fd, void *buf, size_t nbytes);

#endif // UTILS_h_
