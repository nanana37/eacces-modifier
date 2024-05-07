#include <errno.h>
#include <stdio.h>

int main() {
  int x;
  scanf("%d", &x);
  if (x == 0 || x == 1)
    if (x % 2 == 0)
      return -EACCES;
  return 0;
}
