#include <errno.h>
#include <stdio.h>

int errfunc(int x) {

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
    printf("%d ", x);
    errno = errfunc(x);
    if (errno == 0) {
      printf("No error\n");
    } else {
      printf("Error: %d\n", errno);
    }
  }
  return 0;
}
