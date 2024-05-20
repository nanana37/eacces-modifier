#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
int main() {
  mkdir("out", 0777);
  mkdir("out/dir", 1002);

  // change owner
  // uid_t uid = getuid();
  // chown("out/dir", uid, -1);

  // open("out/dir/reg", O_CREAT, 0);

  return 0;
}
