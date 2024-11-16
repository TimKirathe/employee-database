#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "header.h"

#define SERVER_PORT 5555
#define SERVER_ADDRESS "100.68.24.180"
#define BUFLEN 4096
#define MAX_CLIENTS 256

typedef enum {
  STATE_NEW,
  STATE_DISCONNECTED,
  STATE_CONNECTED,
} state_e;

typedef struct {
  int fd;
  state_e state;
  char buffer[BUFLEN];
} client_state_t;

client_state_t client_states[MAX_CLIENTS];

void init_clients() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    client_states[i].fd = -1;
    client_states[i].state = STATE_DISCONNECTED;
    memset(client_states[i].buffer, '\0', BUFLEN);
  }
}

int find_fd_position() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_states[i].fd == -1)
      return i;
  }
  return -1;
}

int main(int argc, char *argv[]) {
  int serverfd = -1;
  int clientfd = -1;
  int ret = -1;
  int max_fds = 0;
  int available_pos = -1;
  unsigned client_addr_size = 0;
  struct sockaddr_in server_addr = {0};
  struct sockaddr_in client_addr = {0};
  struct in_addr server_inaddr = {0};
  fd_set read_fds, write_fds;

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

  ret = listen(serverfd, 10);
  if (ret == -1) {
    perror("listen");
    close(serverfd);
    return -1;
  }

  init_clients();
  while (1) {
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(serverfd, &read_fds);
    max_fds = serverfd + 1;

    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (client_states[i].fd != -1) {
        FD_SET(client_states[i].fd, &read_fds);
        if (client_states[i].fd >= max_fds) {
          max_fds = client_states[i].fd + 1;
        }
      }
    }

    ret = select(max_fds, &read_fds, &write_fds, NULL, NULL);
    if (ret == -1) {
      perror("select");
      close(serverfd);
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_states[i].fd != -1) {
          close(client_states[i].fd);
        }
      }
      return EXIT_FAILURE;
    }

    if (FD_ISSET(serverfd, &read_fds)) {
      clientfd =
          accept(serverfd, (struct sockaddr *)&client_addr, &client_addr_size);
      if (clientfd == -1) {
        perror("accept");
        continue;
      }
      available_pos = find_fd_position();
      if (available_pos != -1) {
        client_states[available_pos].fd = clientfd;
        client_states[available_pos].state = STATE_CONNECTED;
      } else {
        printf("Error: Server has no capacity to accept a new connection\n");
        close(clientfd);
      }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (client_states[i].fd != -1 &&
          FD_ISSET(client_states[i].fd, &read_fds)) {
        ssize_t bytes_read =
            read(client_states[i].fd, client_states[i].buffer, BUFLEN);

        if (bytes_read < 0) {
          perror("read");
          client_states[i].fd = -1;
          client_states[i].state = STATE_DISCONNECTED;
        } else {
          printf("client said %s\n", client_states[i].buffer);
        }
      }
    }
  }

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_states[i].fd != -1) {
      close(client_states[i].fd);
    }
  }
  close(serverfd);

  return EXIT_SUCCESS;
}
