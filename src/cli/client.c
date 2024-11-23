#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "server_header.h"

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

  printf("connected successfully to server. protocol v%d\n", 1);

  return STATUS_SUCCESS;
}
int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <server-ip>\n", argv[0]);
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

  ret = inet_pton(AF_INET, argv[1], &client_inaddr);
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
  send_hello(clientfd);

  close(clientfd);

  return EXIT_SUCCESS;
}
