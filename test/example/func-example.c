#include <errno.h>
#include <stdio.h>

int isOdd(int x) { return x % 2; }

int errfunc(int x) {
  printf("input x = %d\n", x);

  if (!isOdd(x))
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
