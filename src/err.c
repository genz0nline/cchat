#include "err.h"
#include "log.h"

void die(char *s) {
    log_perror(s);
    exit(1);
}
