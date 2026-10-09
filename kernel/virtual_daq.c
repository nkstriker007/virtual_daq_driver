#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

static int __init virtual_daq_init(void)
{
	pr_info("virtual_daq: loaded\n");
	return 0;
}

static void __exit virtual_daq_exit(void)
{
	pr_info("virtual_daq: unloaded\n");
}
module_init(virtual_daq_init);
module_exit(virtual_daq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Narayan Krishna");
MODULE_DESCRIPTION("Virtual data-acquistition device");


