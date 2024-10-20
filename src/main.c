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
  bool new_file = false;
  char *filename = NULL;
  while ((c = getopt(argc, argv, "nf:")) != -1) {
    switch (c) {
    case 'n':
      new_file = true;
      break;
    case 'f':
      filename = optarg;
      break;
    case '?':
      printf("unknown option -%c provided\n", c);
      break;
    default:
      return -1;
    }
  }

  if (!filename) {
    printf("filename is a required argument\n");
    print_usage(argv);
    return EXIT_FAILURE;
  }

  printf("new_file=%d\n", new_file);
  printf("filename=%s\n", filename);

  return EXIT_SUCCESS;
}
