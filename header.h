#ifndef HEADER_H
#define HEADER_H

typedef enum {
  PROTO_HELLO,
} proto_type_e;

// Type Length Value design is used for this protocol.
typedef struct {
  proto_type_e type;
  unsigned short len;
} proto_hdr_t;

#define STATUS_SUCCESS 0
#define STATUS_ERROR -1
#define PROTOCOL_VERSION 1

#endif
