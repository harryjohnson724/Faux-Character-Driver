#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>

#define DEV_MEM_SIZE 512
#undef pr_fmt
#define pr_fmt(fmt) "%s:" fmt,__func__

char device_buffer[DEV_MEM_SIZE];

/* dev_t is uint_32 , a combination of major and minor nums */
dev_t device_number;

struct cdev fcd_cdev;

loff_t fcd_lseek(struct file *filp, loff_t offset, int whence)
{
	loff_t temp;

	pr_info("lseek requested \n");
	pr_info("current file position = %lld\n",filp->f_pos);
	
	switch(whence)
	{
		case SEEK_SET:
			if((offset > DEV_MEM_SIZE) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp > DEV_MEM_SIZE) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = DEV_MEM_SIZE + offset;
			if((temp>DEV_MEM_SIZE) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		default:
			return -EINVAL;
	}

	pr_info("new value of the file pos = %lld\n",filp->f_pos);
	return 0;
}

ssize_t fcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("read requested for %zu bytes \n",count);
	pr_info("Current file pos = %lld\n",*f_pos);

	/*Adjust the count */
	if((*f_pos + count) > DEV_MEM_SIZE)
		count = DEV_MEM_SIZE - *f_pos;

	/* copy to user */
	if(copy_to_user(buff, &device_buffer[*f_pos], count)){
		return -EFAULT;
	}

	/* update the current file pos */
	*f_pos += count;

	pr_info("Number of bytes successfully read= %zu\n", count);
	pr_info("Updated value of f_pos = %lld\n",*f_pos);

	/*Return number of bytes which have been succesfully read */
	return count;
}

ssize_t fcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("write requested for %zu bytes\n", count);	
        pr_info("Current file pos = %lld\n",*f_pos);

        /*Adjust the count */
        if((*f_pos + count) > DEV_MEM_SIZE)
                count = DEV_MEM_SIZE - *f_pos;

	if(!count){
		return -ENOMEM;
	}


        /* copy to user */
        if(copy_from_user(&device_buffer[*f_pos], buff, count)){
                return -EFAULT;
        }

        /* update the current file pos */
        *f_pos += count;

        pr_info("Number of bytes successfully written= %zu\n", count);
        pr_info("Updated value of f_pos = %lld\n",*f_pos);

        /*Return number of bytes which have been succesfully written */
        return count;
}

int fcd_open(struct inode *inode, struct file *filp)
{
	pr_info("open was successful\n");
	return 0;
}

int fcd_release(struct inode *inode, struct file *filp)
{
	pr_info("close was successful\n");
	return 0;
}


/*file operations of the driver */
struct file_operations fcd_fops =
{
	.open = fcd_open,
	.write = fcd_write,
	.read = fcd_read,
	.llseek = fcd_lseek,
	.release = fcd_release,
	.owner = THIS_MODULE
};

struct class *class_fcd;
struct device *device_fcd; 


static int __init fcd_driver_init(void)
{
	int ret;

	/*1. dynamically allocate a device number */
	ret = alloc_chrdev_region(&device_number,0,1,"fcd_devices");
	if(ret < 0)
		goto out;

	pr_info("%s: Device number <major>:<minor> = %d:%d\n",__func__,MAJOR(device_number), MINOR(device_number));

	/*2. Initialize cdev structure with fops */
	cdev_init(&fcd_cdev, &fcd_fops);
	fcd_cdev.owner = THIS_MODULE; //do this only after cdev_init,since init will zero out struct

	/*3. reigster a device(cdev) */
	ret = cdev_add(&fcd_cdev, device_number,1);
	if(ret < 0)
		goto unreg_chrdev;

	/*4. create device class under /sys/class/ */
	class_fcd = class_create(THIS_MODULE,"fcd_class");
	if(IS_ERR(class_fcd))
	{
		pr_err("Class creation failed\n");
		ret = PTR_ERR(class_fcd);
		goto cdev_del;
	}

	/*5. populate the sysfs with device information */
	device_fcd = device_create(class_fcd,NULL,device_number,NULL,"fcd");
	if(IS_ERR(device_fcd))
	{
		pr_err("Device creation failed\n");
		ret = PTR_ERR(device_fcd);
		goto class_del;
	}

	pr_info("Module init was successful");

	return 0;
	
class_del:
	class_destroy(class_fcd);	
cdev_del:
	cdev_del(&fcd_cdev);
unreg_chrdev:
	unregister_chrdev_region(device_number,1);
out:
	return ret;

}

static void __exit fcd_driver_cleanup(void)
{
	/* destroy everything in reverse order what we have done in init func*/
	device_destroy(class_fcd,device_number);
	class_destroy(class_fcd);
	cdev_del(&fcd_cdev);
	unregister_chrdev_region(device_number,1);
	pr_info("module unloaded\n");
}

module_init(fcd_driver_init);
module_exit(fcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HARRY");
MODULE_DESCRIPTION("A faux character driver");
