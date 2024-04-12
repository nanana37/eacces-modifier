#include <linux/printk.h>

void logFunction(char *condition, int caseInt)
{
	pr_info("[PERMOD] %s is %d\n", condition, caseInt);
}
