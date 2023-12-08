#include <stdio.h>
#include <string.h>
#include <errno.h>

char path[100];

int isOK(char* str) {
    int ret = strcmp(str, "OK");
    return strcmp(str, "OK") == 0;
}

int may_open (int mode, int flag) {

    switch (mode) {
        case 1:
            return -ELOOP;
        case 2:
            if (flag & 1) {
                return -EISDIR;
            }
            if (flag & 2) {
                return -EACCES;
            }
            break;
        case 3:
        case 4:
            if (!isOK(path)) {
                return -EACCES;
            }
            /* fallthrough; */
        case 5:
        case 6:
            if (flag & 1) {
                return -EACCES;
            }
            break;
        case 7:
            if ((flag & 1) && isOK(path)) {
                return -EACCES;
            }
        default:
            break;
    }

    return 5;
}


int main(int argc, const char** argv) {

    int mode, flag;
    printf("Enter mode and flag: ");
    scanf("%d %d", &mode, &flag);

    // scan for a pathname
    printf("Enter a pathname: ");
    scanf("%s", path);

    int ret = may_open(mode, flag);

    if (ret < 0) {
        printf("Error: %s\n", strerror(-ret));
    } else {
        printf("Success: %d\n", ret);
    }

    return 0;
}
