#if !defined(USER_MODE)
#include <linux/printk.h>
#define LogFunc(_fmt, ...) pr_debug(_fmt, ##__VA_ARGS__)
#else // USER_MODE
#include <stdio.h>
#define LogFunc(_fmt, ...) fprintf(stderr, _fmt, ##__VA_ARGS__)
#endif

void buffer_cond(long long *ext_list, long long *dst_list, long long nth,
                 long long dest) {
#if defined(DEBUG)
  LogFunc("buffer_cond(%lld, %lld)\n", nth, dest);
#endif
  *ext_list |= (1 << nth);
  if (dest) {
    *dst_list |= (1 << nth);
  } else {
    *dst_list &= ~(1 << nth);
  }
}
#if !defined(USER_MODE)
EXPORT_SYMBOL(buffer_cond);
#endif

void flush_cond(long long *ext_list, long long *dst_list, const char *pathname,
                const char *funcname, int retval) {
  if (retval == -13) {
    LogFunc("[Permod],%s,%s,%d,0x%llx,0x%llx\n",
            pathname,
            funcname,
            retval,
            *ext_list,
            *dst_list);
  }
  *ext_list = 0;
  *dst_list = 0;
}
#if !defined(USER_MODE)
EXPORT_SYMBOL(flush_cond);
#endif
