#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "abuf.h"
#include "state.h"
#include "log.h"
#include "tui.h"
#include "utils.h"

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
    char *leave_chat = "Exit - Ctrl-q";

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

#define MAX_PARTICIPANTS_BUFFER_WIDTH   40

int get_participants_width() {
    int width = C.cols / 4;

    if (width > MAX_PARTICIPANTS_BUFFER_WIDTH) return MAX_PARTICIPANTS_BUFFER_WIDTH;

    return width;
}

void draw_messages(char **mbuf) {
    int width = C.cols - get_participants_width();

    pthread_mutex_lock(&C.messages_mutex);

    for (int buf_idx = MESSAGE_BUF_ROWS - 1; buf_idx >= 0; buf_idx--) {
        mbuf[buf_idx] = malloc(width);

        int mes_idx = C.messages_len - 1 + (buf_idx - MESSAGE_BUF_ROWS + 1) - C.message_offset;
        if (mes_idx >= 0) {
            snprintf(mbuf[buf_idx], width, "%s: %s", C.messages[mes_idx].sender_nickname, C.messages[mes_idx].content);
        } else {
            mbuf[buf_idx][0] = '\0';
        }
    }

    pthread_mutex_unlock(&C.messages_mutex);
}

void draw_participants(char **pbuf) {
    int width = C.cols / 4;
    if (width > MAX_PARTICIPANTS_BUFFER_WIDTH) width = MAX_PARTICIPANTS_BUFFER_WIDTH;

    int n;

    pthread_mutex_lock(&C.participants_mutex);
    int current_participant = 0;
    for (int i = 0; i < C.rows; i++) {
        pbuf[i] = (char *)malloc(width);

        int participants_count = get_participants_count();

        if (i == C.rows - 1) {
            n = snprintf(pbuf[i], 32, "%d %s", participants_count, participants_count == 1 ? "participant" : "participants" );
        } else if (i == C.rows - 2) {
            n = snprintf(pbuf[i], 32, "%d %s", (int) C.messages_len, C.messages_len == 1 ? "message" : "messages" );
        } else if (i < participants_count) {
            while (C.participants[current_participant]->disconnected) current_participant++;
            n = snprintf(pbuf[i], 32, "%s", C.participants[current_participant]->nickname);
            current_participant++;
        } else {
            n = 0;
        }

        for (int j = n; j < width; j++)
            pbuf[i][j] = ' ';
    }
    pthread_mutex_unlock(&C.participants_mutex);
}

void draw_chatroom_ui(abuf *ab) {
    // if (C.mode == CONNECT) {
    //     char *ui = "chatroom ui";
    //     char *mode = C.mode == CONNECT ? "connect" : "host";
    //     draw_centered(ab, ui, strlen(ui));
    //     draw_centered(ab, mode, strlen(mode));
    // } else if (C.mode == HOST) {
        char **pbuf = (char **)malloc(sizeof(char *) * C.rows);
        char **mbuf = (char **)malloc(sizeof(char *) * (C.rows - 1));

        int p_width = get_participants_width();

        draw_participants(pbuf);
        draw_messages(mbuf);

        for (int i = 0; i < C.rows; i++) {
            ab_append(ab, pbuf[i], p_width);
            if (i == C.rows - 1) {
                ab_append(ab, C.current_message, strlen(C.current_message));
            } else {
                ab_append(ab, mbuf[i], strlen(mbuf[i]));
                free(mbuf[i]);
            }
            if (i < C.rows - 1)
                ab_append(ab, "\r\n", 2);
            free(pbuf[i]);

        }
        free(pbuf);
        free(mbuf);
    // }
}

void draw_nickname_negotiation_menu(abuf *ab) {

    int mid = C.rows / 2;
    char buf[64];
    sprintf(buf, "Enter nickname: %s", C.nickname_field);

    for (int i = 0; i < C.rows; i++) {
        if (i == mid) {
            draw_centered(ab, buf, strlen(buf));
        } else {
            ab_append(ab, "\r\n", 2);
        }
    }
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
        case HOST_NICKNAME_NEGOTIATION:
            draw_nickname_negotiation_menu(ab);
            break;
        case PREPARE_CONNECT:
            draw_prepare_connect_menu(ab);
            break;
        case CONNECT_NICKNAME_NEGOTIATION:
            draw_nickname_negotiation_menu(ab);
            break;
        case CONNECT:
            draw_chatroom_ui(ab);
            break;
    }
}

