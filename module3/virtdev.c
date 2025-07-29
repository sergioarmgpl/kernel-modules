#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "virtdev"
#define BUFFER_SIZE 1024

static char message[BUFFER_SIZE];
static size_t message_len = 0;

static ssize_t virtdev_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    if (*offset >= message_len)
        return 0;

    if (len > message_len - *offset)
        len = message_len - *offset;

    if (copy_to_user(buf, message + *offset, len))
        return -EFAULT;

    *offset += len;
    return len;
}

static ssize_t virtdev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    if (len > BUFFER_SIZE)
        len = BUFFER_SIZE;

    if (copy_from_user(message, buf, len))
        return -EFAULT;

    message[len] = '\0';
    message_len = len;

    printk(KERN_INFO "virtdev: received %zu bytes: %s\n", len, message);
    return len;
}

static const struct file_operations virtdev_fops = {
    .owner = THIS_MODULE,
    .read = virtdev_read,
    .write = virtdev_write,
};

static struct miscdevice virtdev_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &virtdev_fops,
    .mode = 0666,
};

static int __init virtdev_init(void) {
    int error = misc_register(&virtdev_device);
    if (error) {
        pr_err("virtdev: failed to register device\n");
        return error;
    }
    pr_info("virtdev: device registered as /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit virtdev_exit(void) {
    misc_deregister(&virtdev_device);
    pr_info("virtdev: device unregistered\n");
}

module_init(virtdev_init);
module_exit(virtdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ChatGPT");
MODULE_DESCRIPTION("Virtual character device for text input/output");

