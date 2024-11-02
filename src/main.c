#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
  printf("Usage: %s -n -f <database file>", argv[0]);
  printf("\t -f  - (required) path to database file\n");
  printf("\t -n  - create new database file\n");
  return;
}

int main(int argc, char *argv[]) {
  int c;
  int dbfd = -1;
  int ret;
  bool new_file = false;
  bool list = false;
  char *filepath = NULL;
  char *employee_to_add = NULL;
  struct dbheader_t *dbhdr = NULL;
  struct employee_t *employees = NULL;

  while ((c = getopt(argc, argv, "nf:a:l")) != -1) {
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
    case 'l':
      list = true;
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
    } else {
      if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
        printf("Failed to create database header\n");
        return -1;
      }
    }
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("unable to open database file\n");
      return -1;
    } else {
      if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
        printf("Failed to validate database header\n");
        return -1;
      }
    }
  }
  ret = read_employees(dbfd, dbhdr, &employees);
  if (ret == STATUS_ERROR) {
    printf("failed to read employees from %s\n", filepath);
    return -1;
  }

  if (employee_to_add) {
    dbhdr->count++;
    if (realloc(employees, dbhdr->count * sizeof(struct employee_t)) == NULL) {
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

  if (list) {
    list_employees(dbhdr, employees);
  }
  output_file(dbfd, dbhdr, employees);

  free(dbhdr);
  free(employees);
  return EXIT_SUCCESS;
}
