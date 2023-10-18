#include <stdio.h>
#include <errno.h>

int my_open (int flag) {

    switch (flag) {
        case 123:
            return -EACCES;
        default:
            break;
    }

    return 5;
}

int main(int argc, const char** argv) {

    int num;
    scanf("%d", &num);
    int ret = my_open(num);

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
