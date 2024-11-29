#include <arpa/inet.h>
#include <bits/getopt_core.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client_header.h"
#include "common.h"
#include "parse.h"
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

  if (dbproto_hdr->type != MSG_HELLO_RESP) {
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

int send_employee_add(int fd, char *employee, size_t employee_size) {
  int ret = -1;
  char buffer[BUFLEN] = {0};

  dbproto_hdr_t *dbproto_hdr = (dbproto_hdr_t *)buffer;
  dbproto_hdr->type = htonl(MSG_EMPLOYEE_ADD_REQ);
  dbproto_hdr->len = htons(1);

  dbproto_employee_add_req *emp_add_req =
      (dbproto_employee_add_req *)&dbproto_hdr[1];

  strncpy(emp_add_req->employee, employee, employee_size);
  printf("employee size is %lu\n", employee_size);

  ret = write(fd, buffer, BUFLEN);
  if (ret < 0) {
    perror("write -> send_employee_add");
    return STATUS_ERROR;
  }
  printf("employee_add_req to server succeded\n");

  // REMINDER: Use gdb to look through what happens in buffer without memset
  // between read and write
  memset(buffer, '\0', BUFLEN);

  ret = read(fd, buffer, BUFLEN);
  if (ret < 0) {
    perror("read -> send_employee_add");
    return STATUS_ERROR;
  }

  dbproto_hdr->type = ntohl(dbproto_hdr->type);
  dbproto_hdr->len = ntohs(dbproto_hdr->len);

  if (dbproto_hdr->type != MSG_EMPLOYEE_ADD_RESP) {
    printf("error send_employee_add: %d\n", dbproto_hdr->type);
    return STATUS_ERROR;
  }

  printf("PROTO_EMPLOYEE_ADD handshake completed successfully. protocol v%d\n",
         PROTOCOL_VERSION);

  return STATUS_SUCCESS;
}

int send_employee_list(int fd) {
  int ret = -1;
  char buffer[BUFLEN] = {'\0'};

  dbproto_hdr_t *dbproto_hdr = (dbproto_hdr_t *)buffer;
  dbproto_hdr->type = htonl(MSG_EMPLOYEE_LIST_REQ);
  dbproto_hdr->len = htons(0);

  ret = write(fd, buffer, BUFLEN);
  if (ret < 0) {
    perror("write -> send_employee_add");
    return STATUS_ERROR;
  }
  printf("employee_list_req successfully sent to the server\n");

  // REMINDER: Use gdb to look through what happens in buffer without memset
  // between read and write
  memset(buffer, '\0', BUFLEN);

  ret = read(fd, buffer, BUFLEN);
  if (ret < 0) {
    perror("read -> send_employee_add");
    return STATUS_ERROR;
  }

  dbproto_hdr->type = ntohl(dbproto_hdr->type);
  dbproto_hdr->len = ntohs(dbproto_hdr->len);

  if (dbproto_hdr->type != MSG_EMPLOYEE_LIST_RESP || dbproto_hdr->len != 2) {
    printf("error send_employee_list: %d\n", dbproto_hdr->type);
    return STATUS_ERROR;
  }

  dbproto_employee_list_resp *emp_list_resp =
      (dbproto_employee_list_resp *)&dbproto_hdr[1];
  emp_list_resp->num_employees = ntohs(emp_list_resp->num_employees);
  struct employee_t *employees = (struct employee_t *)emp_list_resp->employees;
  for (uint32_t i = 0; i < emp_list_resp->num_employees; i++) {
    printf("Employee %u:\n", i);
    printf("\tName: %s\n", employees[i].name);
    printf("\tAddress: %s\n", employees[i].address);
    printf("\tHours worked: %u\n", employees[i].hours);
  }

  printf("PROTO_EMPLOYEE_LIST handshake completed successfully. protocol v%d\n",
         PROTOCOL_VERSION);
  return STATUS_SUCCESS;
}

int main(int argc, char *argv[]) {
  char *server_address = NULL;
  char *employee_to_add = NULL;
  bool list_employees = false;

  int c;
  while ((c = getopt(argc, argv, "h:a:l")) != -1) {
    switch (c) {
    case 'h':
      server_address = optarg;
      break;
    case 'a':
      employee_to_add = optarg;
      break;
    case 'l':
      list_employees = true;
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

  if (employee_to_add) {
    ret = send_employee_add(clientfd, employee_to_add, strlen(employee_to_add));
    if (ret != STATUS_SUCCESS) {
      close(clientfd);
      return -1;
    }
  }

  if (list_employees) {
    ret = send_employee_list(clientfd);
    if (ret != STATUS_SUCCESS) {
      close(clientfd);
      printf("failed to list employees\n");
      return -1;
    }
  }

  close(clientfd);

  return EXIT_SUCCESS;
}
