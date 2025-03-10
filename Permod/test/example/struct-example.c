#include <errno.h>
#include <stdio.h>

struct s {
  int a;
  int b;
};
struct ss {
  struct s s;
  int c;
};

int errfunc(struct ss *ss) {
  struct s *s = &ss->s;
  int x = s->a;
  printf("input x = %d\n", x);

  if (x % 2)
    return -EACCES;

  return 0;
}

int main() {
  int errno;
  struct ss s;
  for (int x = 0; x < 10; x++) {
    s.s.a = x;
    s.s.b = x;
    errno = -errfunc(&s);
    perror("errfunc");
  }
  return 0;
}
