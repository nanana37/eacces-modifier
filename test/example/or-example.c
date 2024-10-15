#include <errno.h>
#include <stdio.h>

int errfunc(int x) {
  printf("input x = %d\n", x);

  if (x & 2 || x & 1) {
    if (x & 4)
      return -EACCES;
  }

  switch (x) {
  case 2:
  case 4:
    return -EACCES;
  case 3:
    printf("Hello, World!\n");
    return -EACCES;
  }
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
