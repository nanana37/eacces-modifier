#include <errno.h>
#include <stdio.h>

static __always_inline int isOdd(int x, int y) {

  if (y == 0)
    return 0;

  return x % 2;
}

int errfunc(int x) {
  printf("input x = %d\n", x);

  if (!isOdd(x, 0))
    return -EACCES;

  return 0;
}

int main() {
  int errno;
  for (int x = 0; x < 10; x++) {
    errno = -errfunc(x);
    perror("errfunc");
  }
  return 0;
}
