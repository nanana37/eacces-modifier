// test.c
#include <stdio.h>

#define HOGE 123
#define FOO(x) ((x) + 1)

// prepare switch case
#define CASE_1 1
#define CASE_2 2
#define CASE_3 3

#define CHECK(x) ((x) > 0)

int main() {
  int a = HOGE;
  int b = FOO(a);
  printf("%d\n", b);

  for (int i = 0; i < 5; i++) {
    if (CHECK(i)) {
      printf("check\n");
    } else {
      printf("not check\n");
    }

    if (i == 0) {
      printf("i is 0\n");
    } else if (i == 1 || i == 4) {
      printf("i is 1\n");
    } else {
      printf("i is not 0 or 1\n");
    }

    switch (i) {
    case CASE_1:
      printf("case 1\n");
      break;
    case CASE_2:
      printf("case 2\n");
      break;
    case CASE_3:
      printf("case 3\n");
      break;
    default:
      printf("default\n");
      break;
    }
  }

  return 0;
}
