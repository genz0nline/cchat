#ifndef PROTO_h_
#define PROTO_h_

#define PROTO_V             1

typedef enum {

    /*** server to client: list of other clients ***/
    STOC_CLILST,

    /*** server to client: another client connected ***/
    STOC_CLICON,

    /*** server to client: another client disconnected ***/
    STOC_CLIDIS,

    /*** server to client: message ***/
    STOC_MSG,

    /*** client to server: message ***/
    CTOS_MSG,

    /*** client to server: change nickname ***/
    CTOS_CNN,

    /*** server to client: change nickname status ***/
    STOC_CNNSTAT,

    /*** server to client: another client changed nickname ***/
    STOC_CNN,
} message_t;

/*** 
 * |  metadata  |        the content         |
 * | <-1 byte-> | <- content-length bytes -> |
 *
 * metadata includes:
 *  - message type (1 byte)
 *  - message length in network byte order (2 bytes)
 *
 * content by type:
 *
 * ------ STOC_CLILST ------
 *  ... ], [
 *  client_id       4 bytes
 *  nickname        32 bytes
 *  ], [ ...
 * 
 * ------ STOC_CLICON ------
 *  client_id       4 bytes
 *  nickname        32 bytes
 * 
 * ------ STOC_CLIDIS ------
 *  client_id       4 bytes
 *
 * ------  STOC_MSG  ------
 *  client_id       4 bytes
 *  message         <var>
 *
 * ------  CTOS_MSG  ------
 *  message         <var>
 *
 * ------  CTOS_CNN  ------
 *  nickname        32 bytes
 * 
 * ----  STOC_CNN_STAT ----
 *  status          1 byte
 *
 * ------  CTOS_CNN  ------
 *  client_id       4 bytes
 *  nickname        32 bytes
 *
 * ***/

#endif // PROTO_h_
