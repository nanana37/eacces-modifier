#include <errno.h>
#include <stdio.h>

int foo(int x) {
  if (x == 2)
    return 0;
  return 1;
}

int main() {
  int x;
  scanf("%d", &x);
  if (!foo(x))
    return -EACCES;
  if (x != 0)
    return -EACCES;
  return 0;
}
