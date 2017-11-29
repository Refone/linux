#include <linux/printk.h>
#include <linux/module.h>

#define MIG_PORT 11603	// network port for migration.

void __request_page(unsigned long address)
{
		    printk("[LRF] migration request page: %lx\n", address);
			    return;
}
EXPORT_SYMBOL_GPL(__request_page);

