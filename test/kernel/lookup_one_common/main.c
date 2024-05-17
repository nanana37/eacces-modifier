#include <fcntl.h>
#include <sys/stat.h>
int main() {
  mknod(".", S_IFREG, 0);
  // open a file
  int fd = open(".", O_RDONLY);

  // close the file
}
