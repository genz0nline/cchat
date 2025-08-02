#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "abuf.h"
#include "state.h"
#include "err.h"
#include <math.h>
#include "proto.h"
#include "tui.h"
#include "utils.h"

extern struct chat_cfg C;

typedef struct Section {
    int rows, cols;
} Section;

struct ui_cfg {
    Section participants;
    Section messages;
    Section statistics;
    Section typing;

    int participants_count;
};

struct ui_cfg U;

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
    pthread_mutex_lock(&C.messages_mutex);
    int n;
    int buf_idx = U.messages.rows - 1 + C.message_offset;
    int mes_idx = C.messages_len - 1;

    while (mes_idx >= 0) {
        char *message = malloc(NN_LEN + MESSAGE_LEN + 2);
        n = sprintf(message, "%s: %s", C.messages[mes_idx].sender_nickname, C.messages[mes_idx].content);
        int rows = (int) ceil((float) strlen(message) / U.messages.cols);

        for (char *p = message + U.messages.cols * (rows - 1); p >= message; p -= U.messages.cols) {
            int len = strlen(p);
            if (0 <= buf_idx && buf_idx < U.messages.rows) {
                memcpy(mbuf[buf_idx], p, U.messages.cols);
                if (len < U.messages.cols) {
                    for (int i = len; i < U.messages.cols; i++)
                        mbuf[buf_idx][i] = ' ';
                }
                buf_idx--;
            }
        }
        free(message);
        mes_idx--;
    }

    while (buf_idx >= 0) {
        for (int i = 0; i < U.messages.cols; i++)
            mbuf[buf_idx][i] = ' ';
        buf_idx--;
    }


    pthread_mutex_unlock(&C.messages_mutex);
}

void draw_participants(char **pbuf) {
    int n;

    pthread_mutex_lock(&C.participants_mutex);
    int current_participant = 0;
    for (int i = 0; i < U.participants.rows; i++) {
        if (i < U.participants_count) {
            while (C.participants[current_participant]->disconnected) current_participant++;
            n = snprintf(pbuf[i], 32, "%s", C.participants[current_participant]->nickname);
            current_participant++;
        } else {
            n = 0;
        }

        for (int j = n; j < U.participants.cols; j++)
            pbuf[i][j] = ' ';
    }
    pthread_mutex_unlock(&C.participants_mutex);
}

void draw_statistics(char **sbuf) {
    int n;
    n = snprintf(sbuf[0], 32, "%d %s", U.participants_count, U.participants_count == 1 ? "participant" : "participants" );
    for (int j = n; j < U.participants.cols; j++)
        sbuf[0][j] = ' ';

    n = snprintf(sbuf[1], 32, "%d %s", (int) C.messages_len, C.messages_len == 1 ? "message" : "messages" );
    for (int j = n; j < U.participants.cols; j++)
        sbuf[1][j] = ' ';
}

void set_section_dimensions() {
    U.participants.cols = C.cols / 4;
    if (U.participants.cols > MAX_PARTICIPANTS_BUFFER_WIDTH)
        U.participants.cols = MAX_PARTICIPANTS_BUFFER_WIDTH;

    U.statistics.cols = U.participants.cols;

    U.messages.cols = C.cols - U.participants.cols - 1;
    U.typing.cols = U.messages.cols;

    U.typing.rows = ceil((float) strlen(C.current_message) / U.typing.cols);
    if (U.typing.rows < 1) 
        U.typing.rows = 1;
    U.statistics.rows = 2;

    U.participants.rows = C.rows - U.statistics.rows - 1;
    U.messages.rows = C.rows - U.typing.rows - 1;

    pthread_mutex_lock(&C.participants_mutex);
    U.participants_count = get_participants_count();
    pthread_mutex_unlock(&C.participants_mutex);

    // log_print("P: %dx%d, M: %dx%d, S: %dx%d, T: %dx%d\n",
    //           U.participants.rows, U.participants.cols,
    //           U.messages.rows, U.messages.cols,
    //           U.statistics.rows, U.statistics.cols,
    //           U.typing.rows, U.typing.cols
    //           );
}

void allocate_buffers(char ***pbuf, char ***mbuf, char ***sbuf, char ***tbuf) {
    int i;
    *pbuf = (char **)malloc(sizeof(char *) * U.participants.rows);
    if (!*pbuf) die("malloc");
    for (i = 0; i < U.participants.rows; i++) {
        (*pbuf)[i] = (char *)malloc(U.participants.cols);
        if (!(*pbuf)[i]) die("malloc");
    }

    *mbuf = (char **)malloc(sizeof(char *) * U.messages.rows);
    if (!*mbuf) die("malloc");
    for (i = 0; i < U.messages.rows; i++) {
        (*mbuf)[i] = (char *)malloc(U.messages.cols);
        if (!(*mbuf)[i]) die("malloc");
    }

    *sbuf = (char **)malloc(sizeof(char *) * U.statistics.rows);
    if (!*sbuf) die("malloc");
    for (i = 0; i < U.statistics.rows; i++) {
        (*sbuf)[i] = (char *)malloc(U.statistics.cols);
        if (!(*sbuf)[i]) die("malloc");
    }

    *tbuf = (char **)malloc(sizeof(char *) * U.typing.rows);
    if (!*tbuf) die("malloc");
    for (i = 0; i < U.typing.rows; i++) {
        (*tbuf)[i] = (char *)malloc(U.typing.cols);
        if (!(*tbuf)[i]) die("malloc");
    }
}

void draw_typing(char **tbuf) {
    char *mes_p = C.current_message;
    int buf_idx = 0;
    int left_len = strlen(C.current_message);
    int left;

    do {
        if (left_len > U.typing.cols) {
            memcpy(tbuf[buf_idx], mes_p, U.typing.cols);
            mes_p += U.typing.cols;
            left_len -= U.typing.cols;
            buf_idx++;
        } else {
            memcpy(tbuf[buf_idx], mes_p, left_len);
            for (int i = left_len; i < U.typing.cols; i++) {
                tbuf[buf_idx][i] = ' ';
            }
            left_len = 0;
        }
    } while (left_len > 0);

}

void draw_chatroom_ui(abuf *ab) {

    char **pbuf, **mbuf, **sbuf, **tbuf;

    set_section_dimensions();
    allocate_buffers(&pbuf, &mbuf, &sbuf, &tbuf);

    draw_participants(pbuf);
    draw_messages(mbuf);
    draw_statistics(sbuf);
    draw_typing(tbuf);

    char *p_divider = malloc(U.participants.cols);
    for (int i = 0; i < U.participants.cols; i++)
        p_divider[i] = '-';

    char *m_divider = malloc(U.messages.cols);
    for (int i = 0; i < U.messages.cols; i++)
        m_divider[i] = '-';

    for (int i = 0; i < C.rows; i++) {
        if (i < U.participants.rows) {
            ab_append(ab, pbuf[i], U.participants.cols);
            free(pbuf[i]);
        } else if (i == U.participants.rows) {
            ab_append(ab, p_divider, U.participants.cols);
            free(p_divider);
        } else {
            ab_append(ab, sbuf[i - U.participants.rows - 1], U.statistics.cols);
            free(sbuf[i - U.participants.rows - 1]);
        }

        ab_append(ab, "|", 1);

        if (i < U.messages.rows) {
            ab_append(ab, mbuf[i], U.messages.cols);
            free(mbuf[i]);
        } else if (i == U.messages.rows) {
            ab_append(ab, m_divider, U.messages.cols);
            free(m_divider);
        } else {
            ab_append(ab, tbuf[i - U.messages.rows - 1], U.typing.cols);
            free(tbuf[i - U.messages.rows - 1]);
        }

        if (i < C.rows - 1)
            ab_append(ab, "\r\n", 2);
    }

    free(pbuf);
    free(mbuf);
    free(sbuf);
    free(tbuf);
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

