#include <errno.h>
#include <stdio.h>

int main() {
  int x;
  scanf("%d", &x);
  if (x == 0 || x == 1)
    return -EACCES;
  return 0;
}
