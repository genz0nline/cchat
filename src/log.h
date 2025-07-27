#ifndef LOG_h_
#define LOG_h_

int log_init();

void log_perror(char *s);
void log_print(char *fmt, ...);
void log_cleanup();

#endif // LOG_h_
