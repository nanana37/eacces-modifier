#include <stdio.h>
#include <errno.h>

int my_open (int flag) {
    switch (flag) {
        case 0:
            return -EACCES;
        default:
            return 0;
    }
}

int main(int argc, const char** argv) {
    int i, num;

    for (i = 0; i < 10; i++) {
        scanf("%i", &num);
        my_open(num);
    }

    return 0;
}
