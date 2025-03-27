#if defined(KERNEL_MODE)
#include <linux/printk.h>
#define LogFunc(_fmt, ...) pr_info(_fmt, ##__VA_ARGS__)
#else // USER_MODE
#include <stdio.h>
#define LogFunc(_fmt, ...) printf(_fmt, ##__VA_ARGS__)
#endif

void buffer_cond(long long *ext_list, long long *dst_list, long long nth,
                 long long dest) {
#ifdef DEBUG2
  LogFunc("buffer_cond(%lld, %lld)\n", nth, dest);
#endif
  *ext_list |= (1 << nth);
  if (dest) {
    *dst_list |= (1 << nth);
  } else {
    *dst_list &= ~(1 << nth);
  }
}
#if defined(KERNEL_MODE)
EXPORT_SYMBOL(buffer_cond);
#endif

void flush_cond(long long *ext_list, long long *dst_list, const char *pathname,
                const char *funcname, int retval) {
  if (retval == -13) {
    LogFunc("============== PERMOD ==============\n");
    LogFunc("[Permod] %s::%s() returned %d\n", pathname, funcname, retval);
    LogFunc("[Permod] (ext)0x%llx, (dst)0x%llx\n", *ext_list, *dst_list);
    LogFunc("====================================\n");
  }
  *ext_list = 0;
  *dst_list = 0;
}
#if defined(KERNEL_MODE)
EXPORT_SYMBOL(flush_cond);
#endif
