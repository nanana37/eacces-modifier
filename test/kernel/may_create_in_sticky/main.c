#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
int main() {
  mkdir("out", 0777);
  mkdir("out/dir", 1777);
  // change dir to out/dir
  // open("out/dir/reg", O_CREAT, 0);
  // mknod("out/dir/reg", S_IFREG, 0);
  chdir("out/dir");

  return 0;
}
