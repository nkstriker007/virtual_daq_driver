#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>

#define VDAQ_NAME "virtaul_daq"
#define VDAQ_DEVICE_COUNT 1
static dev_t vdaq_devno;
static struct cdev vdaq_cdev;
static struct file_operations vdaq_fops={
	.owner=THIS_MODULE,
};
static int __init virtual_daq_init(void)
{
	int ret;
	ret=alloc_chrdev_region(&vdaq_devno,0,VDAQ_DEVICE_COUNT,VDAQ_NAME);
	if(ret<0){
		pr_err("virtual_daq :failed to allocate device no %d\n",ret);
		return ret;
	}
	unsigned int major=MAJOR(vdaq_devno);
	unsigned int minor=MINOR(vdaq_devno);
	pr_info("virtual_daq:loaded ,major %u minor %u \n",major,minor);
	cdev_init(&vdaq_cdev,&vdaq_fops);
	vdaq_cdev.owner=THIS_MODULE;
	ret= cdev_add(&vdaq_cdev,vdaq_devno,VDAQ_DEVICE_COUNT);
	if(ret<0){
		unregister_chrdev_region(vdaq_devno,VDAQ_DEVICE_COUNT);
		pr_err("virtual_daq: failed to register the device %d ",ret);
		return ret;
	}
	pr_info("virtual_daq: sucessfully registed  the device %d",ret);
	pr_info("virtual_daq :sucessfully loaded %d %d ",major,minor);
	
	return 0;
}

static void __exit virtual_daq_exit(void)
{
	cdev_del(&vdaq_cdev);
	unregister_chrdev_region(vdaq_devno,VDAQ_DEVICE_COUNT);
pr_info("virtual_daq: unloaded sucessfully\n");
}

module_init(virtual_daq_init);
module_exit(virtual_daq_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VIRTUAL DAQ SERVER");
MODULE_AUTHOR("Narayan Krishna");
