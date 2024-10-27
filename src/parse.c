#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {}

int add_employee(struct dbheader_t *dbhdr, struct employee_t *employees,
                 char *addstring) {}

int read_employees(int fd, struct dbheader_t *dbhdr,
                   struct employee_t **employeesOut) {}

int output_file(int fd, struct dbheader_t *dbhdr,
                struct employee_t *employees) {

  if (fd < 0) {
    printf("bad file descriptor\n");
    return STATUS_ERROR;
  }

  dbhdr->magic = htonl(dbhdr->magic);
  dbhdr->version = htons(dbhdr->version);
  dbhdr->count = htons(dbhdr->count);
  dbhdr->filesize = htonl(dbhdr->filesize);

  if (lseek(fd, 0, SEEK_SET) < 0) {
    perror("lseek output_file");
    return STATUS_ERROR;
  }

  if (write(fd, dbhdr, sizeof(struct dbheader_t)) < 0) {
    perror("write output_file");
    return STATUS_ERROR;
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
  } else if (dbhdr->magic != HEADER_MAGIC) {
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
