#include "log.h"
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>

pthread_mutex_t log_mutex;

char *log_file_path = NULL;
char *subdir_name = "/.local/state/cchat/";

int create_log_dir_if_not_exist(char *log_dir) {
    DIR *dir = opendir(log_dir);

    if (dir) {
        closedir(dir);
    } else if (errno == ENOENT) {
        if (mkdir(log_dir, 0755)) {
            printf("%s\n", strerror(errno));
            return 1;
        }
    } else {
        printf("%s\n", strerror(errno));
        return 1;
    }

    return 0;
}

#define DATE_LEN    19

char *get_current_time(char delim) {
    time_t t = time(NULL);

    struct tm *tm = localtime(&t);
    char *buf = malloc(DATE_LEN + 1);

    snprintf(buf, DATE_LEN + 1, "%.4d-%.2d-%.2d%c%.2d:%.2d:%.2d",
             tm->tm_year + 1900,
             tm->tm_mon + 1,
             tm->tm_mday,
             delim,
             tm->tm_hour,
             tm->tm_min,
             tm->tm_sec);
    
    return buf;
}


char *extension = ".log";

#define EXTENSION_LEN   strlen(extension)

char *get_log_file_name() {
    char *buf = malloc(DATE_LEN + EXTENSION_LEN + 1);
    char *time = get_current_time('T');

    mempcpy(buf, time, DATE_LEN);
    memcpy(buf + DATE_LEN, extension, EXTENSION_LEN);
    buf[DATE_LEN + EXTENSION_LEN] = '\0';

    free(time);
    return buf;
}

int create_log_file(char *dir, int dev) {

    char *file_name;

    if (dev) {
        file_name = "log.txt";
    } else {
        file_name = get_log_file_name();
    }

    size_t dir_len = strlen(dir);
    size_t file_name_len = strlen(file_name);

    char *path = malloc(dir_len + file_name_len + 1);

    memcpy(path, dir, dir_len);
    memcpy(path + dir_len, file_name, file_name_len);
    path[dir_len + file_name_len] = '\0';
    if (!dev)
        free(file_name);
    
    FILE *fp = fopen(path, "w");
    if (fp) {
        log_file_path = path;
        fclose(fp);
        return 0;
    } else {
        printf("%s\n", strerror(errno));
        return 1;
    }

    return 0;
}

int log_init(int dev) {

    pthread_mutex_init(&log_mutex, NULL);

    if (dev) {
        if (create_log_file("./", dev)) {
            printf("Couldn't create log file\n");
            return 1;
        }

        return 0;
    }

    char *base_log_dir = getenv("HOME");

    size_t base_log_dir_len = strlen(base_log_dir);
    size_t subdir_name_len = strlen(subdir_name);

    if (!base_log_dir || base_log_dir_len == 0) {
        printf("No value for HOME environment variable found\n");
        return 1;
    }

    char *log_dir = malloc(base_log_dir_len + subdir_name_len + 1);
    if (!log_dir) {
        printf("%s\n", strerror(errno));
        return 1;
    }

    memcpy(log_dir, base_log_dir, base_log_dir_len);
    memcpy(log_dir + base_log_dir_len, subdir_name, subdir_name_len);
    log_dir[base_log_dir_len + subdir_name_len] = '\0';

    if (create_log_dir_if_not_exist(log_dir)) {
        printf("Couldn't create or find logging directory\n");
        free(log_dir);
        return 1;
    }

    if (create_log_file(log_dir, dev)) {
        printf("Couldn't create log file\n");
        free(log_dir);
        return 1;
    }

    free(log_dir);
    return 0;
}

void log_cleanup() {
    if (log_file_path) free(log_file_path);
}

void log_print(char *fmt, ...) {
    pthread_mutex_lock(&log_mutex);
    FILE *fp = fopen(log_file_path, "a");
    if (!fp) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    char *time = get_current_time(' ');
    
    fprintf(fp, "[%s] ", time);
    free(time);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);

    fclose(fp);
    pthread_mutex_unlock(&log_mutex);
}

void log_perror(char *s) {
    log_print("%s: %s\n", s, strerror(errno));
}
