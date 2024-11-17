#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
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
    client_states[i].state = STATE_NEW;
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

int find_fd_index(int fd) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_states[i].fd == fd)
      return i;
  }
  return -1;
}

int main(int argc, char *argv[]) {
  int serverfd = -1;
  int clientfd = -1;
  int ret = -1;
  int nfds = 1;
  int available_pos = -1;
  int sockopt = 1;
  unsigned client_addr_size = 0;
  struct sockaddr_in server_addr = {0};
  struct sockaddr_in client_addr = {0};
  struct in_addr server_inaddr = {0};
  struct pollfd all_fds[MAX_CLIENTS + 1];

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

  ret =
      setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
  if (ret == -1) {
    perror("setsockopt");
    close(serverfd);
    return -1;
  }

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

  memset(all_fds, 0, sizeof(all_fds));

  init_clients();
  while (1) {

    all_fds[0].fd = serverfd;
    all_fds[0].events = POLLIN;
    nfds = 1;
    for (int client = 0; client < MAX_CLIENTS; client++) {
      if (client_states[client].fd != -1) {
        all_fds[nfds].fd = client_states[client].fd;
        all_fds[nfds].events = POLLIN;
        nfds++;
      }
    }

    int nevents = poll(all_fds, nfds, -1);
    if (nevents < 0) {
      perror("poll");
      /* Come and release resources properly here */
      return -1;
    }

    if (all_fds[0].revents & POLLIN) {
      nevents--;
      clientfd =
          accept(serverfd, (struct sockaddr *)&client_addr, &client_addr_size);
      if (clientfd == -1) {
        perror("accept");
        /* Come and release resources properly here */
        return -1;
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

    for (int i = 1; i < nfds && nevents > 0; i++) {
      if (all_fds[i].revents & POLLIN) {
        nevents--;
        available_pos = find_fd_index(all_fds[i].fd);
        if (available_pos == -1) {
          continue;
        }
        int bytes_read =
            read(all_fds[i].fd, client_states[available_pos].buffer, BUFLEN);
        if (bytes_read <= 0) {
          perror("read");
          client_states[available_pos].fd = -1;
          client_states[available_pos].state = STATE_DISCONNECTED;
        } else {
          printf("clientfd %d, said %s\n", all_fds[i].fd,
                 client_states[available_pos].buffer);
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
