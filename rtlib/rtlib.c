#include <stdio.h>

void logcase(char *condition, int caseInt) {
    printf("[PERMOD] ");
    printf("%s is %d\n", condition, caseInt);
}

void logOfSwitch(char *condition, int *caseInt) {
    printf("[PERMOD] ");
    printf("%s is %d\n", condition, caseInt);
}
