#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

int main() {
  mkdir("out", 0777);
  return 0;
}
