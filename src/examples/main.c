#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "file.h"
#include "header.h"
#include "parse.h"

clientstate_t client_states[MAX_CLIENTS];

void print_usage(char *argv[]) {
  printf("Usage: %s -n -f <database file>", argv[0]);
  printf("\t -f  - (required) path to database file\n");
  printf("\t -n  - create new database file\n");
  return;
}

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

int poll_loop(struct dbheader_t *dbhdr, struct employee_t *employees) {
  int serverfd = -1;
  int clientfd = -1;
  int sockopt = 1;
  int nfds = 1;
  int available_position = -1;
  int ret = -1;
  unsigned client_addr_size = 0;

  struct pollfd all_fds[MAX_CLIENTS + 1];
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

      available_position = find_fd_position();
      if (available_position != -1) {
        client_states[available_position].fd = clientfd;
        client_states[available_position].state = STATE_CONNECTED;
      } else {
        printf("Error: Server has no capacity to accept a new connection\n");
        close(clientfd);
      }
    }

    for (int i = 1; i < nfds && nevents > 0; i++) {
      if (all_fds[i].revents & POLLIN) {
        nevents--;
        available_position = find_fd_index(all_fds[i].fd);
        if (available_position == -1) {
          continue;
        }
        int bytes_read = read(all_fds[i].fd,
                              client_states[available_position].buffer, BUFLEN);
        if (bytes_read <= 0) {
          perror("read");
          client_states[available_position].fd = -1;
          client_states[available_position].state = STATE_DISCONNECTED;
        } else {
          printf("clientfd %d, said %s\n", all_fds[i].fd,
                 client_states[available_position].buffer);
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {
  int c;
  int dbfd = -1;
  int ret = -1;

  bool new_file = false;
  bool list = false;

  char *filepath = NULL;
  char *employee_to_add = NULL;

  struct dbheader_t *dbhdr = NULL;
  struct employee_t *employees = NULL;

  while ((c = getopt(argc, argv, "nf:a:")) != -1) {
    switch (c) {
    case 'n':
      new_file = true;
      break;
    case 'f':
      filepath = optarg;
      break;
    case 'a':
      employee_to_add = optarg;
      break;
    case '?':
      printf("unknown option -%c provided\n", c);
      break;
    default:
      return -1;
    }
  }

  if (!filepath) {
    printf("filepath is a required argument\n");
    print_usage(argv);
    return EXIT_FAILURE;
  }

  if (new_file) {
    dbfd = create_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("unable to create database file\n");
      return -1;
    }
    if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
      printf("Failed to create database header\n");
      return -1;
    }
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("unable to open database file\n");
      return -1;
    }
    if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
      printf("Failed to validate database header\n");
      return -1;
    }
  }
  ret = read_employees(dbfd, dbhdr, &employees);
  if (ret == STATUS_ERROR) {
    printf("failed to read employees from %s\n", filepath);
    return -1;
  }

  if (employee_to_add) {
    dbhdr->count++;
    employees = realloc(employees, dbhdr->count * sizeof(struct employee_t));
    if (employees == NULL) {
      printf("realloc failed: trying to make space for additional employee\n");
      return -1;
    }
    ret = add_employee(dbhdr, employees, employee_to_add);
    if (ret == STATUS_ERROR) {
      printf("failed to add %s to the %s database file\n", employee_to_add,
             filepath);
      return -1;
    }
  }

  poll_loop(dbhdr, employees);
  output_file(dbfd, dbhdr, employees);

  free(dbhdr);
  free(employees);
  return EXIT_SUCCESS;
}
