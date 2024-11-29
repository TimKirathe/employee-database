#ifndef CLIENT_HEADER_H
#define CLIENT_HEADER_H

#include "common.h"
#include <stddef.h>

int send_hello(int fd);

int send_employee_add(int fd, char *employee, size_t employee_size);

int send_employee_list(int fd);

#endif
