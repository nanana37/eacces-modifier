#include "../permod/include/macro.h"

#ifndef TEST

#include <linux/printk.h>
int counter = 0;

void FUNC_BUF() {
  counter++;
  pr_info("buffer_cond\n");
}

void flush_cond() {
  pr_info("flush_cond\n");
  pr_info("counter: %d\n", counter);
  counter = 0;
}

#else // TEST

#include <stdio.h>

int counter = 0;

void buffer_cond() {
  counter++;
  printf("buffer_cond\n");
}

void flush_cond() {
  printf("flush_cond\n");
  printf("counter: %d\n", counter);
  counter = 0;
}

#endif // TEST
