#ifndef HEADER_H
#define HEADER_H

#include "common.h"
#include "parse.h"

typedef enum {
  PROTO_HELLO,
} proto_type_e;

// Type Length Value design is used for this protocol.
typedef struct {
  proto_type_e type;
  unsigned short len;
} proto_hdr_t;

void init_clients();

int poll_loop(int dbfd, struct dbheader_t *dbhdr, struct employee_t *employees);

int handle_client_fsm(int dbfd, struct dbheader_t *dbhdr,
                      struct employee_t **employees,
                      client_state_t *client_state);

int send_hello_resp_fsm(int clientfd);

int send_employee_add_resp_fsm(int clientfd);

int send_employee_list_resp_fsm(int clientfd, struct dbheader_t *dbhdr,
                                struct employee_t *employees);

int send_client_errmsg(int clientfd, dbproto_error_e error_type);

int find_available_position();

int find_fd_index(int fd);

#endif
