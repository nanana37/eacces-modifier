#include <errno.h>
#include <stdio.h>

int errfunc(int x, int y) {
  printf("input x = %d\n", x);

  if (x & 16) {
    return -EACCES;
  }

  if (x & 8 && !y) {
    return -EACCES;
  }

  if (x & 4) {
    return -EACCES;
  }
  if (x & 4) {
    return -EACCES;
  }
  if (x & 4) {
    return -EACCES;
  }
  if (x & 4) {
    return -EACCES;
  }
  if (x & 4) {
    return -EACCES;
  }
  if (x & 4) {
    return -EACCES;
  }
  if (x & 4) {
    return -EACCES;
  }
  if (x & 4) {
    return -EACCES;
  }
  if (x & 4) {
    return -EACCES;
  }
  if (x & 4) {
    return -EACCES;
  }
  if (x & 4) {
    return -EACCES;
  }

  if (x & 2) {
    return -EACCES;
  }

  return 0;
}

int main() {
  int errno;
  for (int x = 0; x < 10; x++) {
    errno = -errfunc(x, 0);
    perror("errfunc");
  }
  return 0;
}
