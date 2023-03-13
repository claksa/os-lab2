#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/sched/task.h>
#include <linux/netdevice.h>
#include <net/net_namespace.h>
#include <linux/string.h>
#include <linux/spinlock.h>

#include <linux/errno.h>
#include <asm/uaccess.h>

#include "query_ioctl.h"

#define CLASS_NAME "ioctl-driver"
#define DEVICE_NAME "my-ioctl"
#define MINOR_NUM 0
#define MINOR_CNT 1
#define DEV_REG_NAME "ioctl-chrdev"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Polina");
MODULE_DESCRIPTION("my ioctl driver");

static dev_t dev_num;
static struct cdev c_dev;
static struct class *cl;

static char c;
query_cpu_itimer q_it;
net_device_data net_dd;
static arch_spinlock_t spinlock = __ARCH_SPIN_LOCK_UNLOCKED;

static int my_open(struct inode *i, struct file *f)
{
    arch_spin_lock(&spinlock);
    printk(KERN_INFO "my driver: open\n");
    return 0;
}

static int my_close(struct inode *i, struct file *f)
{
    arch_spin_unlock(&spinlock);
    printk(KERN_INFO "my_driver: close\n");
    return 0;
}

static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    struct task_struct *tsk;
    struct cpu_itimer *it;
    struct net* net;
    struct pid *pid;
    struct net_device *net_dev;

    int size = NR_NETD * sizeof(query_net_device);
    net_dd.q_net_list = (query_net_device*) kmalloc(size, GFP_KERNEL);
    if (!net_dd.q_net_list) {
        pr_err("unable to allocate mem for net_dev_list\n");
        return -ENOMEM;
    }

    switch (cmd)
    {
        case QUERY_SET_CPU_ITIMER:
            // 1st -- to, 2nd -- from
            if ( copy_from_user(&q_it, (query_cpu_itimer*) arg, sizeof (query_cpu_itimer)) )
            {
                pr_err("cpu_itimer write fail\n");
                return -EACCES;
            }
            pr_info("cpu_itimer write -- pid_t, type: %d, %d\n", q_it.pid, q_it.cl_id);
            pid = find_get_pid(q_it.pid);
            if (!pid) {
                pr_err("cannot find pid info for %d\n", q_it.pid);
                return -ESRCH;
            }
            tsk = get_pid_task(pid, PIDTYPE_PID);
            if (!tsk) {
                pr_err("cannot find task_struct for pid %d\n", q_it.pid);
                return -ESRCH;
            }
            it = &tsk->signal->it[q_it.cl_id];
            spin_lock_irq(&tsk->sighand->siglock);

            q_it.expires = it->expires;
            q_it.incr = it->incr;

            pr_info("cpu_itimer: expires = %llu, incr = %llu\n", it->expires, it->incr);

            spin_unlock_irq(&tsk->sighand->siglock);
            pr_info("set from kernel struct (expires, incr): %llu, %llu\n", q_it.expires, q_it.incr);
            break;
        case QUERY_GET_CPU_ITIMER:
            pr_info("get cpu_itimer (expires, incr): %llu, %llu\n", q_it.expires, q_it.incr);
            if ( copy_to_user((query_cpu_itimer*) arg, &q_it, sizeof(query_cpu_itimer)) )
            {
                return -EACCES;
            }
            pr_info("cpu_itimer copied to user space\n");
            break;
        case QUERY_SET_NET_DEVICE:
            if ( copy_from_user(&net_dd, (net_device_data*) arg, sizeof(net_device_data)) )
            {
                pr_err("net_device write fail\n");
                return -EACCES;
            }
            net = get_net_ns_by_pid(net_dd.pid);
            if (!net) {
                pr_err("cannot find net struct by pid %d\n", net_dd.pid);
                return -ESRCH;
            }

            int i = 0;
            read_lock(&dev_base_lock);
            for_each_netdev(net, net_dev)
            {
                pr_info("found [%s]\n", net_dev->name);
                net_dd.q_net_list[i].base_addr = net_dev->base_addr;
                net_dd.q_net_list[i].dma = net_dev->dma;
                net_dd.q_net_list[i].if_port = net_dev->if_port;
                net_dd.q_net_list[i].irq = net_dev->irq;
                net_dd.q_net_list[i].mem_end = net_dev->mem_end;
                net_dd.q_net_list[i].mem_start = net_dev->mem_start;
                strcpy(net_dd.q_net_list[i].name, net_dev->name);
                pr_info("found [%s]\n", net_dd.q_net_list[i].name);
                net_dd.q_net_list[i].state = net_dev->state;
                ++i;
            }
            read_unlock(&dev_base_lock);
            if ( copy_to_user( (net_device_data*) arg, &net_dd, sizeof(net_device_data)) )
            {
                kfree(net_dd.q_net_list);
                return -EACCES;
            }
            pr_info("net_device copied to user space\n");
            break;
        case QUERY_GET_NET_DEVICE:
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    pr_info("my driver: read\n");
    if (copy_to_user(buf, &c, 1) != 0)
        return -EFAULT;
    else
        return 1;
}

static ssize_t my_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    pr_info( "my driver: write\n");
    if (copy_from_user(&c, buf + len - 1, 1) != 0)
        return -EFAULT;
    else
        return len;
}

static struct file_operations fops =
        {
            .owner = THIS_MODULE,
            .open = my_open,
            .release = my_close,
            .read = my_read,
            .write = my_write,
            .unlocked_ioctl = my_ioctl
        };

static int __init start(void)
{
    printk(KERN_ALERT "my module registered\n");
    int ret;
    struct device *dev_ret;
    if ( (ret = alloc_chrdev_region(&dev_num, MINOR_NUM, MINOR_CNT, DEV_REG_NAME)) < 0)
    {
        return ret;
    }
    pr_info("<major, minor>: <%d, %d>\n", MAJOR(dev_num), MINOR(dev_num));
    cdev_init(&c_dev, &fops);

    if ( ( ret = cdev_add(&c_dev, dev_num, 1) ) < 0)
    {
        return ret;
    }

    if ( IS_ERR(cl = class_create(THIS_MODULE, CLASS_NAME)) )
    {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev_num, MINOR_CNT);
        return PTR_ERR(cl);
    }
    if ( IS_ERR(dev_ret = device_create(cl, NULL, dev_num, NULL, DEVICE_NAME)) ) {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev_num, MINOR_CNT);
        return PTR_ERR(dev_ret);
    }
    return 0;
}

static void __exit finish(void)
{
    device_destroy(cl, dev_num);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_ALERT "my driver unregistered\n");
}

module_init(start);
module_exit(finish);