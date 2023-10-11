#include <stdio.h>
#include <errno.h>

int my_open (int flag) {
    switch (flag) {
        case 0:
            printf("flag is 0\n");
            return -EACCES;
        default:
            return 0;
    }
}

int main(int argc, const char** argv) {

    my_open(0);

    return 0;
}
