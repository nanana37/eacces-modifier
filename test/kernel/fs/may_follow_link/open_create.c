#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int fd;
  if (argc != 2) {
    fprintf(stderr, "usage: my_open <file>\n");
    exit(1);
  }
  fd = open(argv[1], O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    perror("open");
    exit(1);
  }
  close(fd);
  return 0;
}
