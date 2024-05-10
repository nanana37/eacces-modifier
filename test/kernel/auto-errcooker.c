/* Very simple cat */

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include <linux/fs.h>
#define MAY_EXEC 0x00000010

void check_imode(char *filename) {
  struct stat st;
  stat(filename, &st);
  printf("imode: %07o ", st.st_mode & S_IFMT);

  switch (st.st_mode & S_IFMT) {
  case S_IFIFO:
    printf("IFIFO\n");
    break;
  case S_IFCHR:
    printf("IFCHR\n");
    break;
  case S_IFDIR:
    printf("IFDIR\n");
    break;
  case S_IFBLK:
    printf("IFBLK\n");
    break;
  case S_IFREG:
    printf("IFREG\n");
    break;
  case S_IFLNK:
    printf("IFLNK\n");
    break;
  case S_IFSOCK:
    printf("IFSOCK\n");
    break;
  default:
    printf("Unknown\n");
    break;
  }
}
int main(int argc, char *argv[]) {
  // For landmark
  getpid();

  char *pathname = "";
  char *const args[] = {pathname, NULL};

  int mode = 0;
  while (1) {
    mode++;
    getpid();
    switch (mode) {
    case 0:
      printf("Program exit\n");
      return 0;
    case 1:
      // S_IFLNK
      pathname = "lnk";
      symlink("lnk", "lnk");
      break;
    case 2:
      // S_IFDIR
      pathname = "dir";
      mkdir(pathname, 0777);
      break;
    case 3:
      // S_IFBLK
      printf("isNODEV:");
      int isNODEV = 0;
      scanf("%d", &isNODEV);
      if (isNODEV) {
        mkdir("nodev", 0777);
        // sudo mount -o nodev -t tmpfs tmpfs ./nodev
        mount("nodev", "nodev", "tmpfs", MS_NODEV, NULL);
        pathname = "nodev/blk";
      } else {
        pathname = "blk";
      }
      mknod(pathname, S_IFBLK | 0666, makedev(1, 1));
      break;
    case 4:
      // S_IFCHR
      printf("isNODEV:");
      scanf("%d", &isNODEV);
      if (isNODEV) {
        mkdir("nodev", 0777);
        // sudo mount -o nodev -t tmpfs tmpfs ./nodev
        mount("nodev", "nodev", "tmpfs", MS_NODEV, NULL);
        pathname = "nodev/chr";
      } else {
        pathname = "chr";
      }
      mknod(pathname, S_IFCHR | 0777, 0);
      break;
    case 5:
      // S_IFIFO
      pathname = "fifo";
      mkfifo(pathname, 0777);
      break;
    case 6:
      // S_IFSOCK
      pathname = "sock";
      mknod(pathname, S_IFSOCK | 0777, 0);
      break;
    case 7:
      // S_IFREG
      mkdir("noexec", 0777);
      /* mknod ("reg", S_IFREG | 0777, 0); */
      // After mount as noexec
      // sudo mount -o noexec -t tmpfs tmpfs ./noexec
      mount("noexec", "noexec", "tmpfs", MS_NOEXEC, NULL);
      pathname = "noexec/reg";
      mknod(pathname, S_IFREG | 0777, 0);
      break;
    default:
      pathname = "";
      printf("Invalid mode\n");
      exit(1);
    }
    if (pathname != "") {
      check_imode(pathname);
      execv(pathname, args);
      perror(pathname);
    }
  }
  return 0;
}
