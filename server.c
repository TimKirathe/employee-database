#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "header.h"

#define SERVER_PORT 5555
#define SERVER_ADDRESS "100.68.24.180"

int handle_clientfd(int fd) {
  int ret = -1;
  char buffer[4096] = {0};
  proto_hdr_t *hdr = (proto_hdr_t *)buffer;
  hdr->type = htonl(PROTO_HELLO);
  int real_len = sizeof(int);
  hdr->len = htons(real_len);

  int *value = (int *)&hdr[1];
  *value = htonl(PROTOCOL_VERSION);
  ret = write(fd, hdr, sizeof(proto_hdr_t) + real_len);
  if (ret == -1) {
    perror("write - handle_clientfd()");
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

int main(int argc, char *argv[]) {
  int serverfd = -1;
  int clientfd = -1;
  int ret = -1;
  unsigned client_addr_size = 0;
  struct sockaddr_in server_addr = {0};
  struct sockaddr_in client_addr = {0};
  struct in_addr server_inaddr = {0};

  serverfd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverfd == -1) {
    perror("socket");
    return -1;
  }
  printf("serverfd is %d\n", serverfd);

  ret = inet_pton(AF_INET, SERVER_ADDRESS, &server_inaddr);
  if (ret == 0) {
    printf("invalid sever address\n");
    close(serverfd);
    return -1;
  } else if (ret == -1) {
    perror("inet_pton");
    close(serverfd);
    return -1;
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = server_inaddr.s_addr;
  server_addr.sin_port = htons(SERVER_PORT);

  ret = bind(serverfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (ret == -1) {
    perror("bind");
    close(serverfd);
    return -1;
  }

  ret = listen(serverfd, 0);
  if (ret == -1) {
    perror("listen");
    close(serverfd);
    return -1;
  }

  while (1) {
    clientfd =
        accept(serverfd, (struct sockaddr *)&client_addr, &client_addr_size);
    if (clientfd == -1) {
      perror("accept");
      close(clientfd);
      close(serverfd);
      return EXIT_FAILURE;
    }

    if (handle_clientfd(clientfd) == STATUS_ERROR) {
      printf("failed to write to client file descriptor\n");
      close(clientfd);
      close(serverfd);
      return EXIT_FAILURE;
    }

    close(clientfd);
  }
  close(serverfd);

  return EXIT_SUCCESS;
}
