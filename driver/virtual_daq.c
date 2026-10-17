#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/jiffies.h>

#define VDAQ_NAME "virtual_daq"
#define VDAQ_DEVICE_COUNT 1

static dev_t vdaq_devno;
static struct cdev vdaq_cdev;
static struct class *vdaq_class;
static struct device *vdaq_device;
static int fail_step;

struct vdaq_device
{
	spinlock_t lock; /*guards the data below*/
	struct timer_list timer;
	unsigned int period_ms;
	u32 sample_index; /* shared state*/
	s32 latest_sample; /* shared state*/
};
static struct vdaq_device vdaq;
static int vdaq_open(struct inode *inode, struct file *file)
{
	file->private_data=&vdaq;
	pr_info("device opened\n");
	return stream_open(inode,file);
}

static int vdaq_release(struct inode *inode,struct file *file){
	pr_info("device released\n");
	return 0;
}

static void vdaq_timer_fn(struct timer_list *t)
{
	struct vdaq_device *dev = timer_container_of(dev, t, timer);
	spin_lock(&dev->lock);
	dev->latest_sample=100000+(dev->sample_index %100000);
	dev->sample_index++;
	spin_unlock(&dev->lock);
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->period_ms));
}
static ssize_t vdaq_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	s32 sample;
	if(count< sizeof(sample)) {
		return -EINVAL;
	}
	struct vdaq_device *dev = file->private_data;
	spin_lock_bh(&dev->lock);
	sample=dev->latest_sample;
	spin_unlock_bh(&dev->lock);
	if(copy_to_user(buf,&sample,sizeof(sample))) {
		return -EFAULT;
	}
	pr_debug("device read\n");
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
	spin_lock_init(&vdaq.lock);
	timer_setup(&vdaq.timer, vdaq_timer_fn, 0);
	vdaq.period_ms = 100; /* default period */
	ret=alloc_chrdev_region(&vdaq_devno,0,VDAQ_DEVICE_COUNT,VDAQ_NAME);
	if(ret<0){
		pr_err("failed to allocate device no %d\n",ret);
		return ret;
	}
	if (fail_step == 1) {  
		ret=-ENOMEM;                 /* test hook */
		goto err_unregister;
	}
	unsigned int major=MAJOR(vdaq_devno);
	unsigned int minor=MINOR(vdaq_devno);

	cdev_init(&vdaq_cdev,&vdaq_fops);
	vdaq_cdev.owner=THIS_MODULE;
	ret=cdev_add(&vdaq_cdev,vdaq_devno,VDAQ_DEVICE_COUNT);
	if(ret<0){
		pr_err("failed to register the device %d ",ret);
		goto err_unregister;
	}
	if (fail_step == 2) {  
		ret=-ENOMEM;                 /* test hook */
		goto err_cdev_del;
	}
	vdaq_class=class_create(VDAQ_NAME);
	if(IS_ERR(vdaq_class)) {
		ret=PTR_ERR(vdaq_class);
		pr_err("failed to create class\n");
		goto err_cdev_del;
	}
	if (fail_step == 3) { 
		ret=-ENOMEM;                  /* test hook */
		goto err_class_destroy;
	}
	
	vdaq_device=device_create(vdaq_class,NULL,vdaq_devno,NULL,"vdaq%d",MINOR(vdaq_devno));
	if(IS_ERR(vdaq_device)) {
		ret=PTR_ERR(vdaq_device);
		pr_err("failed to create device\n");
		goto err_class_destroy;
	}
	if (fail_step == 4) {                   /* test hook */
		ret = -ENOMEM;
		goto err_device_destroy;
	}

	pr_info("sucessfully registed  the device %d",ret);
	pr_info("sucessfully loaded %d %d ",major,minor);
	mod_timer(&vdaq.timer, jiffies + msecs_to_jiffies(vdaq.period_ms));
	return 0;
	err_device_destroy:
		device_destroy(vdaq_class, vdaq_devno);
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
	timer_shutdown_sync(&vdaq.timer);
	device_destroy(vdaq_class, vdaq_devno);
	class_destroy(vdaq_class);
	cdev_del(&vdaq_cdev);
	unregister_chrdev_region(vdaq_devno,VDAQ_DEVICE_COUNT);
	pr_info("unloaded sucessfully\n");
}

module_init(virtual_daq_init);
module_exit(virtual_daq_exit);
module_param(fail_step, int, 0444);

MODULE_PARM_DESC(fail_step, "Testing only: force init to fail after step N (1-4)");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("VIRTUAL DAQ SERVER");
MODULE_AUTHOR("Narayan Krishna");
