// X42.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc > 1 && strcmp(argv[1], "42") == 0) {
    fprintf(stderr, "It depends!\n");
    exit(42);
  }
  // strcmp(argv[0], "42");
  strcmp(argv[0], "42");
  fprintf(stderr, "stderr: %d\n", argc);
  printf("What is the meaning of life?\n");
  exit(0);
}