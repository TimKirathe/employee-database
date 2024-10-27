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
  bool new_file = false;
  char *filepath = NULL;
  struct dbheader_t *dbhdr = NULL;

  while ((c = getopt(argc, argv, "nf:")) != -1) {
    switch (c) {
    case 'n':
      new_file = true;
      break;
    case 'f':
      filepath = optarg;
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
      printf("%s created successfully\n", filepath);
      if (create_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
        printf("Failed to create database header\n");
        return -1;
      }
    }
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("unable to create database file\n");
      return -1;
    } else {
      printf("%s opened successfully\n", filepath);
      if (validate_db_header(dbfd, &dbhdr) == STATUS_ERROR) {
        printf("Failed to validate database header\n");
        return -1;
      }
    }
  }

  printf("new_file=%d\n", new_file);
  printf("filepath=%s\n", filepath);

  output_file(dbfd, dbhdr, struct employee_t * employees);

  free(dbhdr);
  return EXIT_SUCCESS;
}
