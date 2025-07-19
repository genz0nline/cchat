#include <stdio.h>
#include <string.h>
#include "abuf.h"
#include "state.h"
#include "tui.h"

extern struct chat_cfg C;

void draw_metrics(abuf *ab) {
    char buf[16];
    int len = snprintf(buf, 16, "%dx%d\r\n", C.cols, C.rows);
    ab_append(ab, buf, len);
}

void draw_centered(abuf *ab, char *s, int len) {
    int left_len = C.cols;
    for (int j = 0; j < (C.cols - len) / 2; j++) {
        ab_append(ab, " ", 1);
        left_len--;
    }
    ab_append(ab, s, len);
    left_len -= len;
    while (left_len--) {
        ab_append(ab, " ", 1);
    }
    ab_append(ab, "\r\n", 2);
}

void draw_mode_menu(abuf *ab) {
    int host_idx = C.rows / 2 - 1;
    int connect_idx = host_idx + 1;
    int exit_idx = host_idx + 2;

    char *host_chat = "Host a new chat room - h";
    char *connect_to_chat = "Connect to existing chat room - c";
    char *leave_chat = "Exit - q";

    for (int i = 0; i < C.rows; i++) {
        if (i == host_idx) {
            draw_centered(ab, host_chat, strlen(host_chat));
        } else if (i == connect_idx) {
            draw_centered(ab, connect_to_chat, strlen(connect_to_chat));
        } else if (i == exit_idx) {
            draw_centered(ab, leave_chat, strlen(leave_chat));
        } else {
            ab_append(ab, "\r\n", 2);
        }
    }
}

void draw_prepare_host_menu(abuf *ab) {
    char *ui = "prepare host";
    draw_centered(ab, ui, strlen(ui));
}

void draw_prepare_connect_menu(abuf *ab) {
    char *ui = "prepare connect";
    draw_centered(ab, ui, strlen(ui));
}

void draw_chatroom_ui(abuf *ab) {
    char *ui = "chatroom ui";
    char *mode = C.mode == CONNECT ? "connect" : "host";
    draw_centered(ab, ui, strlen(ui));
    draw_centered(ab, mode, strlen(mode));
}

void draw_interface(abuf *ab) {

    switch (C.mode) {
        case UNDEFINED:
            draw_mode_menu(ab);
            break;
        case PREPARE_HOST:
            draw_prepare_host_menu(ab);
            break;
        case HOST:
            draw_chatroom_ui(ab);
            break;
        case PREPARE_CONNECT:
            draw_prepare_connect_menu(ab);
            break;
        case CONNECT:
            draw_chatroom_ui(ab);
            break;
    }
}

