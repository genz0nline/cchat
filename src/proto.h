#ifndef PROTO_h_
#define PROTO_h_

#include <stddef.h>
#include <stdint.h>

#define PROTO_VER          1

/*** -------- Protocol description -------- ***/

    /*** -------- Message types -------- ***/

typedef uint8_t message_t;

/*** server to client: list of other clients ***/
#define STOC_CLILST         0x01

/*** server to client: another client connected ***/
#define STOC_CLICON         0x02

/*** server to client: another client disconnected ***/
#define STOC_CLIDIS         0x03

/*** server to client: message ***/
#define STOC_MSG            0x04

/*** client to server: message ***/
#define CTOS_MSG            0x05

/*** client to server: change nickname ***/
#define CTOS_CNN            0x06

/*** server to client: change nickname status ***/
#define STOC_STAT           0x07

/*** server to client: another client changed nickname ***/
#define STOC_CNN            0x08

/*** client to server: introduction with a nickname ***/
#define CTOS_INTRO          0x09

    /*** -------- Packaging -------- ***/

#define PV_LEN              1
#define MT_LEN              1
#define CL_LEN              2
#define MD_LEN              PV_LEN + MT_LEN + CL_LEN

#define NN_LEN              32
#define ID_LEN              2

#define STAT_LEN            1
#define STAT_SUCCESS        0x00
#define STAT_INVALID        0x01
#define STAT_TAKEN          0x02

/*** 
 * |                      metadata                    |                            |
 * | protocol version | message type | content length |          content           |
 * | <--- 1 byte ---> | <- 1 byte -> | <- 2 bytes  -> |                            |
 * | <------------------ 4 bytes -------------------> | <- content-length bytes -> |
 *
 * metadata includes:
 *  - message type (1 byte)
 *  - message length in network byte order (2 bytes)
 *
 * content by type:
 *
 * ------ STOC_CLILST ------
 *  ... ], [
 *  client_id       2 bytes
 *  nickname        32 bytes
 *  ], [ ...
 * 
 * ------ STOC_CLICON ------
 *  client_id       2 bytes
 *  nickname        32 bytes
 * 
 * ------ STOC_CLIDIS ------
 *  client_id       2 bytes
 *
 * ------  STOC_MSG  ------
 *  client_id       2 bytes
 *  message         <var>
 *
 * ------  CTOS_MSG  ------
 *  message         <var>
 *
 * ------  CTOS_CNN  ------
 *  nickname        32 bytes
 * 
 * ------  STOC_STAT ------
 *  status          1 byte
 *
 * ------  CTOS_CNN  ------
 *  client_id       4 bytes
 *  nickname        32 bytes
 *
 * ------ CTOS_INTRO ------
 *  nickname        32 bytes
 *
 * ***/

/*** declarations ***/

typedef struct Client Client;

char *form_message(message_t type, Client *client, char *content, size_t *message_len);
void process_message(message_t type, char *content, uint16_t content_length, Client *client, uint8_t *status);
void server_broadcast_message(message_t type, Client *client, char *content);
void server_send_message(message_t type, Client* client, char *content);

#endif // PROTO_h_
