#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "server_header.h"

void print_usage(char *argv[]) {
  printf("Usage: %s -h <server address> -a <employee to add>", argv[0]);
  printf("\t -h  - (required) ip address of server\n");
  printf(
      "\t -a  - comma separated string containing fields of employee to add\n");
  return;
}

int send_hello(int fd) {
  int ret = -1;
  char buffer[4096] = {0};

  dbproto_hdr_t *dbproto_hdr = (dbproto_hdr_t *)buffer;
  dbproto_hdr->type = htonl(MSG_HELLO_REQ);
  dbproto_hdr->len = htons(1);

  dbproto_hello_req *hello_req = (dbproto_hello_req *)&dbproto_hdr[1];
  hello_req->proto = htons(PROTOCOL_VERSION);

  ret = write(fd, buffer, BUFLEN);
  if (ret < 0) {
    perror("write -> send_hello");
    return STATUS_ERROR;
  }
  printf("hello_req to server succeded\n");

  // REMINDER: Use gdb to look through what happens in buffer without memset
  // between read and write
  memset(buffer, '\0', BUFLEN);

  ret = read(fd, buffer, BUFLEN);
  if (ret < 0) {
    perror("read -> send_hello");
    return STATUS_ERROR;
  }

  dbproto_hdr->type = ntohl(dbproto_hdr->type);
  dbproto_hdr->len = ntohs(dbproto_hdr->len);

  if (dbproto_hdr->type == MSG_PROTOCOL_VER_ERR) {
    dbproto_hello_resp *hello_resp = (dbproto_hello_resp *)&dbproto_hdr[1];
    if (hello_resp->proto != PROTOCOL_VERSION) {
      printf("error: protocol mismatch\n");
    } else {
      printf("error: unknown (at send_hello dbproto_hdr->type)\n");
    }
    return STATUS_ERROR;
  }

  printf("PROTO_HELLO handshake completed successfully. protocol v%d\n",
         PROTOCOL_VERSION);

  return STATUS_SUCCESS;
}
int main(int argc, char *argv[]) {
  char *server_address = NULL;
  char *employee_to_add = NULL;

  int c;
  while ((c = getopt(argc, argv, "h:a:")) != -1) {
    switch (c) {
    case 'h':
      server_address = optarg;
      break;
    case 'a':
      employee_to_add = optarg;
      break;
    case '?':
      printf("unknown option %c provided\n", c);
      break;
    default:
      return -1;
    }
  }

  if (!server_address) {
    printf("server address is a required argument\n");
    print_usage(argv);
    return -1;
  }

  int clientfd = -1;
  int ret = -1;
  unsigned client_addr_size = 0;
  struct sockaddr_in server_addr = {0};
  struct in_addr client_inaddr;

  clientfd = socket(AF_INET, SOCK_STREAM, 0);
  if (clientfd == -1) {
    perror("socket");
    return -1;
  }
  printf("clientfd is %d\n", clientfd);

  ret = inet_pton(AF_INET, server_address, &client_inaddr);
  if (ret == 0) {
    printf("invalid sever address\n");
    close(clientfd);
    return -1;
  } else if (ret == -1) {
    perror("inet_pton");
    close(clientfd);
    return -1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = client_inaddr.s_addr;
  server_addr.sin_port = htons(SERVER_PORT);

  ret = connect(clientfd, (struct sockaddr *)&server_addr,
                sizeof(struct sockaddr_in));

  if (ret == -1) {
    perror("connect");
    close(clientfd);
    return -1;
  }

  printf("connected successfully to server\n");

  ret = send_hello(clientfd);
  if (ret != STATUS_SUCCESS) {
    close(clientfd);
    return -1;
  }

  close(clientfd);

  return EXIT_SUCCESS;
}
