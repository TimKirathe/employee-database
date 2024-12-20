#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
  int i = 0;
  for (; i < dbhdr->count; i++) {
    printf("employee %d\n", i);
    printf("\tname: %s\n", employees[i].name);
    printf("\taddress: %s\n", employees[i].address);
    printf("\thours: %d\n", employees[i].hours);
  }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees,
                 char *addstring) {
  printf("%s\n", addstring);
  char *employee_name = strtok(addstring, ",");
  if (employee_name == NULL)
    return STATUS_ERROR;
  char *employee_addr = strtok(NULL, ",");
  if (employee_addr == NULL)
    return STATUS_ERROR;
  char *employee_hours = strtok(NULL, ",");
  if (employee_hours == NULL)
    return STATUS_ERROR;

  printf("%s %s %s\n", employee_name, employee_addr, employee_hours);

  dbhdr->count++;
  *employees = realloc(*employees, dbhdr->count * sizeof(struct employee_t));
  if (*employees == NULL) {
    printf("realloc failed: trying to make space for additional employee\n");
    return STATUS_ERROR;
  }
  struct employee_t *employee_ptr =
      *employees; /* Easier to work with regular pointer than double pointer */

  strncpy(employee_ptr[dbhdr->count - 1].name, employee_name,
          sizeof(employee_ptr[dbhdr->count - 1].name));
  strncpy(employee_ptr[dbhdr->count - 1].address, employee_addr,
          sizeof(employee_ptr[dbhdr->count - 1].address));
  employee_ptr[dbhdr->count - 1].hours = atoi(employee_hours);

  return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr,
                   struct employee_t **employeesOut) {
  if (fd < 0) {
    printf("bad file descriptor\n");
    return STATUS_ERROR;
  }

  struct employee_t *employees =
      calloc(dbhdr->count, sizeof(struct employee_t));
  if (employees == NULL) {
    printf("employees calloc failed\n");
    free(dbhdr);
    return STATUS_ERROR;
  }

  if (read(fd, employees, dbhdr->count * sizeof(struct employee_t)) < 0) {
    perror("read read_employees");
    free(employees);
    free(dbhdr);
    return STATUS_ERROR;
  }

  for (int i = 0; i < dbhdr->count; i++) {
    employees[i].hours = ntohl(employees[i].hours);
  }

  *employeesOut = employees;

  return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr,
                struct employee_t *employees) {

  if (fd < 0) {
    printf("bad file descriptor\n");
    return STATUS_ERROR;
  }

  int count = dbhdr->count;

  dbhdr->magic = htonl(dbhdr->magic);
  dbhdr->filesize =
      htonl(sizeof(struct dbheader_t) + (count * sizeof(struct employee_t)));
  dbhdr->version = htons(dbhdr->version);
  dbhdr->count = htons(dbhdr->count);

  if (lseek(fd, 0, SEEK_SET) < 0) {
    perror("lseek output_file");
    return STATUS_ERROR;
  }

  if (write(fd, dbhdr, sizeof(struct dbheader_t)) < 0) {
    perror("write output_file - dbhdr");
    return STATUS_ERROR;
  }

  dbhdr->magic = ntohl(dbhdr->magic);
  dbhdr->filesize = ntohl(dbhdr->filesize);
  dbhdr->version = ntohs(dbhdr->version);
  dbhdr->count = ntohs(dbhdr->count);

  for (int empl = 0; empl < count; empl++) {
    employees[empl].hours = htonl(employees[empl].hours);
    if (write(fd, &employees[empl], sizeof(struct employee_t)) < 0) {
      perror("write output_file - employee_t");
      return STATUS_ERROR;
    }
    employees[empl].hours = ntohl(employees[empl].hours);
  }
  return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  if (fd < 0) {
    printf("bad file descriptor\n");
    return STATUS_ERROR;
  }

  struct dbheader_t *dbhdr = calloc(1, sizeof(struct dbheader_t));
  if (dbhdr == NULL) {
    printf("error - calloc failed\n");
    return STATUS_ERROR;
  }

  if (read(fd, dbhdr, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
    perror("read");
    free(dbhdr);
    return STATUS_ERROR;
  }

  dbhdr->version = ntohs(dbhdr->version);
  dbhdr->count = ntohs(dbhdr->count);
  dbhdr->magic = ntohl(dbhdr->magic);
  dbhdr->filesize = ntohl(dbhdr->filesize);

  if (dbhdr->version != 1) {
    printf("invalid version number\n");
    free(dbhdr);
    return STATUS_ERROR;
  }
  if (dbhdr->magic != HEADER_MAGIC) {
    printf("invalid header magic\n");
    free(dbhdr);
    return STATUS_ERROR;
  }

  struct stat dbhdr_stat = {0};
  fstat(fd, &dbhdr_stat);
  if (dbhdr->filesize != dbhdr_stat.st_size) {
    printf("corrupted database header file\n");
    free(dbhdr);
    return STATUS_ERROR;
  }

  *headerOut = dbhdr;

  return STATUS_SUCCESS;
}

int create_db_header(int fd, struct dbheader_t **headerOut) {
  struct dbheader_t *dbhdr = calloc(1, sizeof(struct dbheader_t));
  if (dbhdr == NULL) {
    printf("error - calloc failed\n");
    return STATUS_ERROR;
  }

  dbhdr->magic = HEADER_MAGIC;
  dbhdr->version = 0x1;
  dbhdr->count = 0;
  dbhdr->filesize = sizeof(struct dbheader_t);

  *headerOut = dbhdr;

  return STATUS_SUCCESS;
}
