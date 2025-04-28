#define EACCES 13
#define HOGE 1
#define FOO(x) ((x) + 1)

// prepare switch case
#define CASE_1 1
#define CASE_2 2
#define CASE_3 3

#define CHECK(x) ((x) > 0)

int func_M(int i) {
  if (CHECK(i)) {
    return -EACCES;
  }
  return 0;
}

int main() {
  func_M(1);
  return 0;
}