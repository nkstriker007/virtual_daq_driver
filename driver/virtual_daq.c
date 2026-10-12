#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define VDAQ_NAME "virtual_daq"
#define VDAQ_DEVICE_COUNT 1
static dev_t vdaq_devno;
static struct cdev vdaq_cdev;
static struct class *vdaq_class;
static struct device *vdaq_device;
static int vdaq_open(struct inode *inode, struct file *file)
{
	pr_info("virtual_daq: device opened\n");
	return stream_open(inode,file);
}
static int vdaq_release(struct inode *inode,struct file *file){
	pr_info("virtual_daq:device released\n");
	return 0;
}
static ssize_t vdaq_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	s32 sample=123000;
	if(count< sizeof(sample))
		return -EINVAL;
	if(copy_to_user(buf,&sample,sizeof(sample)))
		return -EFAULT;
	pr_debug("virtual_daq: device read\n");
	return sizeof(sample);
}
static const struct file_operations vdaq_fops={
	.owner=THIS_MODULE,
	.open=vdaq_open,
	.release=vdaq_release,
	.read=vdaq_read,
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

	cdev_init(&vdaq_cdev,&vdaq_fops);
	vdaq_cdev.owner=THIS_MODULE;
	ret= cdev_add(&vdaq_cdev,vdaq_devno,VDAQ_DEVICE_COUNT);
	if(ret<0){
		pr_err("virtual_daq: failed to register the device %d ",ret);
		goto err_unregister;
	}
	vdaq_class=class_create(VDAQ_NAME);
	if(IS_ERR(vdaq_class)) {
		ret=PTR_ERR(vdaq_class);
		pr_err("virtual_daq: failed to create class\n");
		goto err_cdev_del;
	}
	vdaq_device=device_create(vdaq_class,NULL,vdaq_devno,NULL,"vdaq%d",MINOR(vdaq_devno));
	if(IS_ERR(vdaq_device)) {
		ret=PTR_ERR(vdaq_device);
		pr_err("virtual_daq: failed to create device\n");
		goto err_class_destroy;
	}

	pr_info("virtual_daq: sucessfully registed  the device %d",ret);
	pr_info("virtual_daq :sucessfully loaded %d %d ",major,minor);
	
	return 0;
	err_class_destroy:
		class_destroy(vdaq_class);
	err_cdev_del:
		cdev_del(&vdaq_cdev);
	err_unregister:
		unregister_chrdev_region(vdaq_devno, VDAQ_DEVICE_COUNT);
		return ret;
}

static void __exit virtual_daq_exit(void)
{
	device_destroy(vdaq_class, vdaq_devno);
	class_destroy(vdaq_class);
	cdev_del(&vdaq_cdev);
	unregister_chrdev_region(vdaq_devno,VDAQ_DEVICE_COUNT);
	pr_info("virtual_daq: unloaded sucessfully\n");
}

module_init(virtual_daq_init);
module_exit(virtual_daq_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VIRTUAL DAQ SERVER");
MODULE_AUTHOR("Narayan Krishna");
