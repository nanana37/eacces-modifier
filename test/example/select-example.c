#include <errno.h>
#include <stdio.h>

int errfunc(int x) { return x % 2 == 0 ? 0 : -EACCES; }

int main() {
  int errno;
  for (int x = 0; x < 4; x++) {
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
