#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define STATUS_ERROR -1
#define STATUS_SUCCESS 0
#define PROTOCOL_VERSION 100

#define SERVER_PORT 5555
#define SERVER_ADDRESS "100.68.24.180"
#define BUFLEN 4096
#define MAX_CLIENTS 256

typedef enum {
  STATE_NEW,
  STATE_DISCONNECTED,
  STATE_CONNECTED,
  STATE_HELLO,
  STATE_MSG,
  STATE_GOODBYE,
} state_e;

typedef struct {
  int fd;
  state_e state;
  char buffer[BUFLEN];
} client_state_t;

typedef enum {
  MSG_PROTOCOL_VER_ERR,
} dbproto_error_e;

typedef enum {
  MSG_HELLO_REQ,
  MSG_HELLO_RESP,
  MSG_EMPLOYEE_LIST_REQ,
  MSG_EMPLOYEE_LIST_RESP,
  MSG_EMPLOYEE_ADD_REQ,
  MSG_EMPLOYEE_ADD_RESP,
  MSG_EMPLOYEE_DEL_REQ,
  MSG_EMPLOYEE_DEL_RESP,
} dbproto_type_e;

typedef struct {
  dbproto_type_e type;
  uint16_t len;
} dbproto_hdr_t;

typedef struct {
  uint16_t proto;
} dbproto_hello_req;

typedef struct {
  uint64_t proto;
} dbproto_hello_resp;

void print_usage(char *argv[]);

#endif
