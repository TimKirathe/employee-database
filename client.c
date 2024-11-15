#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "header.h"

#define SERVER_PORT 5555

int handle_clientfd(int fd) {
  char buffer[4096] = {0};
  proto_hdr_t *hdr = (proto_hdr_t *)buffer;
  read(fd, hdr, sizeof(proto_hdr_t) + sizeof(int));

  hdr->type = ntohl(hdr->type);
  hdr->len = ntohs(hdr->len);

  int *value = (int *)&hdr[1];
  *value = ntohl(*value);

  if (hdr->type != PROTO_HELLO) {
    printf("Invalid protocol type. Required is %d, but was %d\n", PROTO_HELLO,
           hdr->type);
    return STATUS_ERROR;
  } else if (*value != PROTOCOL_VERSION) {
    printf("Invalid protocol length. Required is %lu, but was %d\n",
           sizeof(*value), hdr->len);
    return STATUS_ERROR;
  }

  printf("Protocol v1 successfully established! Connected to server\n");
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
  printf("serverfd is %d\n", clientfd);

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
  handle_clientfd(clientfd);

  close(clientfd);

  return EXIT_SUCCESS;
}
