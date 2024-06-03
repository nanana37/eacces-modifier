#include <errno.h>
#include <stdio.h>

int errfunc(int x) {
  printf("x = %d\n", x);

  if (x & 1)
    return -EACCES;
  return -EACCES;
}

int main() {
  for (int i = 0; i < 6; i++) {
    int ret = errfunc(i);
    switch (ret) {
    case -EACCES:
      printf("EACCES\n");
      break;
    default:
      printf("default\n");
      break;
    }
  }
}
