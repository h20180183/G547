#include<linux/module.h>
#include<linux/kdev_t.h>
#include<linux/types.h>
#include<linux/device.h>
#include<linux/cdev.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/random.h>
#include<linux/errno.h>
#include<linux/uaccess.h>

static dev_t device_num;
static struct class* adxl_axes;
static struct cdev* cdev_ptr;

static char* device_names[3] = {"adxl_x","adxl_y","adxl_z"};

// This function is used to change the permissions of the device files created
static int perm_uevent(struct device* dev,struct kobj_uevent_env* env)
{
  add_uevent_var(env,"DEVMODE=%#o",0666);
  return 0;
}

static int open_func(struct inode* i,struct file* filp)
{
 printk(KERN_INFO"Open()\n");
 return 0;
}

static int close_func(struct inode* i, struct file* filp)
{
 printk(KERN_INFO"close()\n");
 return 0;
}

static ssize_t read_func(struct file* filp, char __user *buf,size_t s,loff_t* t)
{
  static int i;
   get_random_bytes(&i,2);
   i &= (0x03ff);
   if(copy_to_user(buf,&i,4))
   {
    printk(KERN_INFO"Read action failed\n");
    return -EFAULT;
   }
  printk(KERN_INFO"Read action is done");
 return s;
}

static ssize_t write_func(struct file* filp, const char __user*buf, size_t s, loff_t* t)
{
 printk(KERN_INFO"write()\n");
 return s;
}

static struct file_operations fops =
					{
					  .owner = THIS_MODULE,
					  .open = open_func,
				    .release = close_func,
					  .read = read_func,
	          .write = write_func
					};

static int __init myinit_func(void)
{
 int i=0;
 // Alocation of device numbers.
 if(alloc_chrdev_region(&device_num,0,3,"ADXL") < 0)
 {
  return -1;
 }
 printk(KERN_INFO"ADXL is registered\n");
 printk(KERN_INFO"<Major,Minor>:<%d,%d>\n",MAJOR(device_num),MINOR(device_num));
 //Class creation
if ((adxl_axes = class_create(THIS_MODULE,"adxl_axes"))==NULL)
{
 printk(KERN_INFO"Couldn't create the class\n");
 unregister_chrdev_region(device_num,3);
 printk(KERN_INFO"ADXL is unregistered\n");
 return -1;
}
printk(KERN_INFO"Class created\n");
adxl_axes->dev_uevent = perm_uevent;
for (i =0;i<=2;i++)
{
 if (device_create(adxl_axes,NULL,MKDEV(MAJOR(device_num),MINOR(device_num)+i),NULL,device_names[i])==NULL)
 {
  printk(KERN_INFO"Coudn't create the device files\n");
  class_destroy(adxl_axes);
  printk(KERN_INFO"Class destroyed\n");
  unregister_chrdev_region(device_num,3);
  printk(KERN_INFO"ADXL is unregistered\n");
  return -1;
 }
 printk(KERN_INFO"Device %s created\n",device_names[i]);
}
 //cdev structure creation and linking file_operations.
 cdev_ptr = cdev_alloc();
 cdev_init(cdev_ptr,&fops);
 cdev_add(cdev_ptr,device_num,3);
 printk(KERN_INFO"cdev,file_operations created\n");
 return 0;
}

static void __exit myexit_func(void)
{
 int i=0;
 cdev_del(cdev_ptr);
 for(i=0;i<=2;i++)
{
 device_destroy(adxl_axes,MKDEV(MAJOR(device_num),MINOR(device_num)+i));
}
 class_destroy(adxl_axes);
 unregister_chrdev_region(device_num,3);
 printk(KERN_INFO"ADXL is unregistered\n");
}


module_init(myinit_func);
module_exit(myexit_func);

MODULE_AUTHOR("Perumalla Deepak");
MODULE_DESCRIPTION("Assignment-1");
MODULE_LICENSE("GPL");
