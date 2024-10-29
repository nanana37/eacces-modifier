#include "../permod/include/macro.h"

#ifndef TEST
#include <linux/printk.h>
#define LogFunc(_fmt, ...) pr_info(_fmt, ##__VA_ARGS__)
#else
#include <stdio.h>
#define LogFunc(_fmt, ...) printf(_fmt, ##__VA_ARGS__)
#endif // TEST

long long existList = 0;
long long destList = 0;

void buffer_cond(long long nth, long long dest) {
#ifdef DEBUG2
  LogFunc("buffer_cond(%lld, %lld)\n", nth, dest);
#endif
  existList |= (1 << nth);
  if (dest) {
    destList |= (1 << nth);
  }
}

void flush_cond(int retval) {
  if (retval != 0) {
    LogFunc("retval: %d\n", retval);
    LogFunc("(ext)0x%llx, (dst)0x%llx\n", existList, destList);
  }
  existList = 0;
  destList = 0;
}
