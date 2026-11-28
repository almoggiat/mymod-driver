#include <linux/module.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/string.h>

struct my_priv {
    char memory[4096];
    int tail;
    int head;
    int statistics[2];
};
enum {
    OPEN_COUNT = 0,
    CLOSE_COUNT = 1,
};

static struct my_priv device_data = {0};
static struct cdev cdev1;
static dev_t dev_num;
static struct class *cl1;
static ssize_t lp_read(struct file *file, char __user *buffer,
                       size_t count, loff_t *offset)
{
    struct my_priv *priv = file->private_data;
    size_t available = priv->head - priv->tail;

    if (count > available)
        count = available;

    if (count == 0)
        return 0;

    if (copy_to_user(buffer, priv->memory + priv->tail, count))
        return -EFAULT;

    priv->tail += count;

    if (priv->tail == priv->head) {
        priv->tail = 0;
        priv->head = 0;
    }

    return count;
}

static ssize_t lp_write(struct file *file, const char __user *buffer,
                        size_t count, loff_t *offset)
{
    struct my_priv *priv = file->private_data;
    size_t space = sizeof(priv->memory) - priv->head;
    size_t unread;

    if (count > space && priv->tail > 0) {
        unread = priv->head - priv->tail;
        memmove(priv->memory, priv->memory + priv->tail, unread);

        priv->tail = 0;
        priv->head = unread;
        space = sizeof(priv->memory) - priv->head;
    }

    if (count > space)
        count = space;

    if (count == 0)
        return 0;

    if (copy_from_user(priv->memory + priv->head, buffer, count))
        return -EFAULT;

    priv->head += count;
    return count;
}

static int lp_open(struct inode *inode, struct file *file)
{
    struct my_priv *priv = &device_data;

    file->private_data = priv;
    priv->statistics[OPEN_COUNT]++;

    return 0;
}

static int lp_release(struct inode *inode, struct file *file)
{
    struct my_priv *priv = file->private_data;

    priv->statistics[CLOSE_COUNT]++;
    return 0;
}
static long lp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct my_priv *priv = file->private_data;

    if (cmd != _IO('L', 1))
        return -ENOTTY;

    printk(KERN_INFO "mymod: opens=%d, releases=%d\n",
           priv->statistics[OPEN_COUNT],
           priv->statistics[CLOSE_COUNT]);

    return 0;
}

static const struct file_operations fops1 = {
    .owner          = THIS_MODULE,
    .read           = lp_read,
    .write          = lp_write,
    .open           = lp_open,
    .release        = lp_release,
    .unlocked_ioctl = lp_ioctl,
};

static int __init mymod_init(void)
{
    struct device *dev1;
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, "mymod");
    if (ret)
        return ret;

    cdev_init(&cdev1, &fops1);
    cdev1.owner = THIS_MODULE;

    ret = cdev_add(&cdev1, dev_num, 1);
    if (ret)
        goto err_unregister;

    cl1 = class_create("class1");
    if (IS_ERR(cl1)) {
        ret = PTR_ERR(cl1);
        goto err_cdev;
    }

    dev1 = device_create(cl1, NULL, dev_num, NULL, "device1");
    if (IS_ERR(dev1)) {
        ret = PTR_ERR(dev1);
        goto err_class;
    }
    printk(KERN_INFO "mymod: module loaded\n");
    return 0;
err_class:
    class_destroy(cl1);
    
err_cdev:
    cdev_del(&cdev1);
err_unregister:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit mymod_exit(void)
{
    device_destroy(cl1, dev_num);
    class_destroy(cl1);
    cdev_del(&cdev1);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "mymod: module unloaded\n");
}

module_init(mymod_init);
module_exit(mymod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
