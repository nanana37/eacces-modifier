#include <stdio.h>
#include <errno.h>

int my_open (int flag) {

    switch (flag) {
        case 123:
            printf("flag is 123\n");
            return -EACCES;
        case 456:
            printf("flag is 456\n");
            return -EACCES;
        default:
            break;
    }

    return 5;
}

int main(int argc, const char** argv) {

    int num;
    scanf("%d", &num);
    my_open(num);

    /* switch (num) { */
    /*     case 0: */
    /*         printf("num is 0\n"); */
    /*         return -EACCES; */
    /*     default: */
    /*         break; */
    /* } */

    return 0;
}
