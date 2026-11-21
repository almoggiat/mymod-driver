#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/device.h>

struct cdev cdev1;
static dev_t dev_num;
static ssize_t lp_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t lp_write(struct file *, const char __user *, size_t, loff_t *);
static int lp_open(struct inode *, struct file *);
static int lp_release(struct inode *, struct file *);
static const struct file_operations fops1 = {
    .owner   = THIS_MODULE,
    .read    = lp_read,
    .write   = lp_write,
    .open    = lp_open,
    .release = lp_release,
};
static struct class *cl1;
static struct device *dev1;

static int __init mymod_init(void){
  alloc_chrdev_region(&dev_num, 0, 1, "mymod");
  cdev_init (&cdev1, &fops1);
  cdev_add (&cdev1, dev_num, 1);
  cl1 = class_create(THIS_MODULE, "class1");
  dev1 = device_create(cl1, NULL, dev_num, NULL, "device1");
  printk(KERN_INFO "module loaded\n");
  return 0;
}


static void __exit mymod_exit(void){
  printk(KERN_INFO "module unloaded\n");
  device_destroy(cl1, dev_num);
  class_destroy (cl1);
  cdev_del (&cdev1);
}

module_init(mymod_init);
module_exit(mymod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
