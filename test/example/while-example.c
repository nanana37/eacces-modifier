#include <errno.h>
#include <stdio.h>

int errfunc(int x) {
  int i = 0;
  while (i < 5) {
    printf("%d\n", i);
    i++;
  }
  if (x & 0) {
    return -EACCES;
  }
  return 0;
}

int main() {
  for (int i = 0; i < 5; i++) {
    errfunc(i);
  }
}
