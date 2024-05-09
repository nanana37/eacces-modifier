#include <errno.h>
#include <stdio.h>

int errfunc(int x) {

  if (x == 0 || x == 1)
    return -EACCES;

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
  for (int x = 0; x < 4; x++) {
    errno = errfunc(x);
    if (errno == 0) {
      printf("No error\n");
    } else {
      printf("Error: %d\n", errno);
    }
  }
  return 0;
}
