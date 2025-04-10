// test.c
#include <stdio.h>

#define EACCES 13

#define HOGE 1
#define FOO(x) ((x) + 1)

// prepare switch case
#define CASE_1 1
#define CASE_2 2
#define CASE_3 3

#define CHECK(x) ((x) > 0)

int func(int i) {
  if (CHECK(i)) {
    printf("check\n");
  } else {
    printf("not check\n");
  }

  if (i == HOGE) {
    printf("i is HOGE\n");
  } else if (i == FOO(i)) {
    printf("i is FOO\n");
  }

  switch (i) {
  case CASE_1:
    printf("case 1\n");
  case CASE_2:
    printf("case 2\n");
    return -EACCES;
  case CASE_3:
    printf("case 3\n");
    return -EACCES;
  default:
    printf("default\n");
    break;
  }

  return 0;
}

int main() {
  int a = HOGE;
  int b = FOO(a);
  printf("%d\n", b);

  for (int i = 0; i < 5; i++) {
    func(i);
  }
  return 0;
}