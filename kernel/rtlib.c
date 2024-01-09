#include <linux/printk.h>

void logcase(char *condition, int caseInt) {
    printk(KERN_INFO "[PERMOD] %s: %d\n", condition, caseInt);
}
