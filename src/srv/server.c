#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "file.h"
#include "parse.h"
#include "server_header.h"

client_state_t client_states[MAX_CLIENTS];

void handle_sigint(int signal) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (client_states[i].fd != -1) {
      close(client_states[i].fd);
    }
  }
  printf("Exiting...\n");
}

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

int send_client_errmsg(int clientfd) {
  int ret = -1;
  char buffer[4096];

  dbproto_hdr_t *dbproto_hdr = (dbproto_hdr_t *)buffer;
  dbproto_hdr->type = htonl(MSG_PROTOCOL_VER_ERR);
  dbproto_hdr->len = htons(1);

  dbproto_hello_resp *hello_resp = (dbproto_hello_resp *)&dbproto_hdr[1];
  hello_resp->proto = htons(PROTOCOL_VERSION);

  ret = write(clientfd, buffer, BUFLEN);
  if (ret == -1) {
    perror("write -> send_client_errmsg");
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

int send_hello_resp_fsm(int clientfd) {
  int ret = -1;
  char buffer[4096];

  dbproto_hdr_t *dbproto_hdr = (dbproto_hdr_t *)buffer;
  dbproto_hdr->type = htonl(MSG_HELLO_RESP);
  dbproto_hdr->len = htons(1);

  dbproto_hello_resp *hello_resp = (dbproto_hello_resp *)&dbproto_hdr[1];
  hello_resp->proto = htons(PROTOCOL_VERSION);

  ret = write(clientfd, buffer, BUFLEN);
  if (ret == -1) {
    perror("write -> send_hello_resp_fsm");
    return STATUS_ERROR;
  }
  return STATUS_SUCCESS;
}

int handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t *employees,
                      client_state_t *client_state) {
  int ret = -1;
  dbproto_hdr_t *dbproto_hdr = (dbproto_hdr_t *)client_state->buffer;

  dbproto_hdr->type = ntohl(dbproto_hdr->type);
  dbproto_hdr->len = ntohs(dbproto_hdr->len);

  switch (client_state->state) {
  case STATE_HELLO:
    if (dbproto_hdr->type != MSG_HELLO_REQ || dbproto_hdr->len != 1) {
      printf("error: client in STATE_HELLO, but either didn't receive "
             "MSG_HELLO_REQ or length of message > 1\n");
      // TODO: Handle error messages in a better way
      send_client_errmsg(client_state->fd);
    }

    dbproto_hello_req *hello_req = (dbproto_hello_req *)&dbproto_hdr[1];
    hello_req->proto = ntohs(hello_req->proto);

    if (hello_req->proto != PROTOCOL_VERSION) {
      printf("error: client protocol version %d is different from current one "
             "%d\n",
             hello_req->proto, PROTOCOL_VERSION);
      // TODO: Handle error messages in a better way
      send_client_errmsg(client_state->fd);
    }
    ret = send_hello_resp_fsm(client_state->fd);
    if (ret != STATUS_SUCCESS) {
      printf("error sending hello response to client %d\n",
             client_state->state);
      return STATUS_ERROR;
    }
    client_state->state = STATE_MSG;
    printf("client %d state updated to %d\n", client_state->fd, STATE_MSG);
    return STATUS_SUCCESS;
  case STATE_MSG:
    // Add Logic Here
    return STATUS_SUCCESS;
  default:
    printf("unknown client state %d\n", client_state->state);
    return STATUS_ERROR;
  }
}

int poll_loop(struct dbheader_t *dbhdr, struct employee_t *employees) {
  int serverfd = -1;
  int clientfd = -1;
  int sockopt = 1;
  int nfds = 1;
  int available_position = -1;
  int ret = -1;
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

  init_clients();
  memset(all_fds, 0, sizeof(struct pollfd) * (MAX_CLIENTS + 1));

  all_fds[0].events = POLLIN;
  all_fds[0].fd = serverfd;
  while (1) {
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
        client_states[available_position].state = STATE_HELLO;
        printf("client %d successfully connected\n", clientfd);
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
        if (bytes_read < 0) {
          perror("read");
          printf("client disconnected with bytes_read < 0\n");
        } else if (bytes_read == 0) {
          printf("client %d disconnected\n",
                 client_states[available_position].fd);
          client_states[available_position].fd = -1;
          client_states[available_position].state = STATE_DISCONNECTED;
          memset(client_states[available_position].buffer, '\0', BUFLEN);
        } else {
          handle_client_fsm(dbhdr, employees,
                            &client_states[available_position]);
          /* printf("clientfd %d, said %s\n", all_fds[i].fd, */
          /*        client_states[available_position].buffer); */
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
  struct sigaction signal_action;

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

  memset(&signal_action, 0, sizeof(struct sigaction));

  signal_action.sa_handler = handle_sigint;
  signal_action.sa_flags = 0;
  sigemptyset(&signal_action.sa_mask);

  if (sigaction(SIGINT, &signal_action, NULL) == -1) {
    perror("sigaction");
    return -1;
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
