#include <errno.h>
#include <stdio.h>

int errfunc(int x) {
  printf("input x = %d\n", x);

  if (!x) {
    // 0
    return -EACCES;
  } else if (x == 1) {
    // 1
    return -EACCES;
  } else {
    return -EACCES;
  }

  if (x < 1) {
    // 0
    return -EACCES;
  }
  if (x > 4) {
    // 4, 5
    return -EACCES;
  }
  if (x >= 4) {
    // 3
    return -EACCES;
  }
  if (x == 2) {
    // 2
    return -EACCES;
  }
  return -EACCES;
  return 0;
}

int main() {
  for (int i = 0; i < 6; i++) {
    errno = -errfunc(i);
    perror("errfunc");
  }
}
