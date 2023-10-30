#include <stdio.h>
#include <errno.h>

int my_open (int mode, int flag) {

    switch (mode) {
        case 123:
            if (flag & 1) {
                return -EACCES;
            }
            break;
        case 456:
            if (!(flag & 1)) {
                return -EACCES;
            }
            break;
        default:
            break;
    }

    return 5;
}

int main(int argc, const char** argv) {

    int mode, flag;
    printf("Enter mode and flag: ");
    scanf("%d %d", &mode, &flag);

    int ret = my_open(mode, flag);

    switch (ret) {
        case -EACCES:
            printf("Permission denied\n");
            break;
        default:
            printf("my_open suceeded\n");
            break;
    }

    return 0;
}
