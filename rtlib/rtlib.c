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
#ifndef TEST
EXPORT_SYMBOL(buffer_cond);
#endif

void flush_cond(const char *pathname, const char *funcname, int retval) {
  if (retval == -13) {
    LogFunc("============== PERMOD ==============\n");
    LogFunc("[Permod] %s::%s() returned %d\n", pathname, funcname, retval);
    LogFunc("[Permod] (ext)0x%llx, (dst)0x%llx\n", existList, destList);
    LogFunc("====================================\n");
  }
  existList = 0;
  destList = 0;
}
#ifndef TEST
EXPORT_SYMBOL(flush_cond);
#endif
