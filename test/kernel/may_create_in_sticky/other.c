#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
int main() {
  getuid();
  int err = open("out/dir/reg", O_CREAT | O_WRONLY, 0644);
  if (err < 0) {
    perror("open");
  }
  return 0;
}
