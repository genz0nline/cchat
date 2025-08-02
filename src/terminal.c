#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>

#include "terminal.h"
#include "abuf.h"
#include "err.h"
#include "log.h"
#include "state.h"
#include "tui.h"

static struct termios orig_termios;

extern struct chat_cfg C;

void hide_cursor() {
    abuf ab = ab_init();

    ab_append(&ab, "\x1b[?25L", 6);

    ab_flush(&ab);
    ab_free(&ab);
}

void show_cursor() {
    abuf ab = ab_init();

    ab_append(&ab, "\x1b[?25H", 6);

    ab_flush(&ab);
    ab_free(&ab);
}


void disable_raw_mode() {
    clean_screen();

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        log_perror("tcsetattr");
    }
}

void enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        log_perror("tcgetattr");
        exit(1);
    }

    atexit(disable_raw_mode);

    struct termios raw = orig_termios;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        log_perror("tcsetattr");
        exit(1);
    }

    hide_cursor();
}

void update_screen_size() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        die("ioctl");
    }
    C.cols = ws.ws_col;
    C.rows = ws.ws_row;
}

void clean_screen() {
    abuf ab = ab_init();

    ab_append(&ab, "\x1b[2J", 4);
    ab_append(&ab, "\x1b[H", 3);

    ab_flush(&ab);
    ab_free(&ab);
}

void refresh_screen() {
    update_screen_size();

    abuf ab = ab_init();

    ab_append(&ab, "\x1b[2J", 4);
    ab_append(&ab, "\x1b[H", 3);

    draw_interface(&ab);

    ab_flush(&ab);
    ab_free(&ab);

}
