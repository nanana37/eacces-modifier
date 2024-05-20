#include <errno.h>
#include <stdio.h>

int errfunc(int x) {
  printf("x = %d\n", x);
  int i = 0;
  while (i != 5) {
    i++;
    if (x == 2) {
      return -EACCES;
    }
  }
  if (x & 1) {
    return -EACCES;
  }
  return 0;
}

int main() {
  for (int i = 0; i < 3; i++) {
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
