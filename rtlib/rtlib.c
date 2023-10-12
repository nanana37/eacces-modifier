#include <stdio.h>
#include <errno.h>

void print_detailed_error (int errno)
{
    switch (errno) {
        case -EACCES:
            printf ("I found \"Permission denied\"!\n");
            break;
        default:
            printf ("I found something else!\n");
            break;
    }
}

