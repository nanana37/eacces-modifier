#include <stdio.h>
#include <errno.h>

int may_open (int mode, int flag) {

    switch (mode) {
        case 123:
            if (flag & 2) {
                return -EACCES;
            }
            break;
        case 456:
            if (!(flag & 2)) {
                return -EACCES;
            }
            break;
        case 789:
            if (flag & 2) {
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

    int ret = may_open(mode, flag);

    switch (ret) {
        case -EACCES:
            printf("Permission denied\n");
            break;
        default:
            printf("may_open suceeded\n");
            break;
    }

    return 0;
}
