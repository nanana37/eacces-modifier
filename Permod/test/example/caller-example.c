#include <errno.h>
#include <stdio.h>

int isOdd(int x, int y) { return x % 2; }

int errfunc(int x) {
  printf("input x = %d\n", x);

  if (!isOdd(x, 0))
    return -EACCES;

  return 0;
}

int caller(int x) {
  if (x > 0) {
    return errfunc(x);
  }
  return 0;
}

int main() {
  int errno;
  for (int x = 0; x < 10; x++) {
    errno = -caller(x);
    perror("errfunc");
  }
  return 0;
}
