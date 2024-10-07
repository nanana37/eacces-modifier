#include <errno.h>
#include <stdio.h>

int errfunc(int x, int y) {
  int err = 0;

  if (x != 0) {
    goto out;
  }

  err = -EACCES;
  if (y == 0) {
    goto out;
  }

  err = -EPERM;

out:
  return err;
}

int main() {
  int errno;
  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 2; y++) {
      printf("x: %d y: %d\n", x, y);
      errno = errfunc(x, y);

      if (errno == 0) {
        printf("No error\n");
      } else {
        printf("Error: %d\n", errno);
      }
    }
  }
  return 0;
}
