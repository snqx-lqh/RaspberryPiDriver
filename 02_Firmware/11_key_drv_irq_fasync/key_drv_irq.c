/*
 * 这个驱动文件测试，树莓派使用中断检测按键，并且实现按键中断printk一些东西
 * 加入定时器模块实现按键检测的延时判断，实现消抖
 * 使用树莓派gpio子系统的gpiod开头的api控制对应的引脚电平  
 * 使用timer相关函数，注意在新版本中setup_timer等已经弃用了，维护代码的时候需要注意。
 * 
 * + 这部分代码使用Fasync机制
 */
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/poll.h>

typedef struct{
    int key_num;
    struct gpio_desc *key_gpiod;
    struct timer_list key_timer;
    int key_irq;
}key_gpio_t;

typedef struct{
    dev_t devid; /* 设备号 */
    struct cdev cdev; /* cdev */
    struct device_node *node; /* KEY 设备节点 */
    key_gpio_t key_gpio[3];
}key_dev_t;

static key_dev_t key_dev;

static int g_key = 0;
static struct fasync_struct *button_fasync;

static DECLARE_WAIT_QUEUE_HEAD(gpio_key_wait);

// 打开文件
static int key_drv_open(struct inode *inode, struct file *file)
{
	printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

// 关闭文件
static int key_drv_release(struct inode *inode, struct file *file)
{
	printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

ssize_t key_drv_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
    int err;
    wait_event_interruptible(gpio_key_wait, g_key);
	err = copy_to_user(buf, &g_key, 4);
	g_key = 0;
    return -EINVAL;
}

static unsigned int  key_drv_poll(struct file *fp, poll_table * wait)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	poll_wait(fp, &gpio_key_wait, wait);
	return g_key ? (POLLIN | POLLRDNORM) : 0;
}

static int key_drv_fasync(int fd, struct file *file, int on)
{
	if (fasync_helper(fd, file, on, &button_fasync) >= 0)
		return 0;
	else
		return -EIO;
}

ssize_t key_drv_write(struct file* filp, const char __user* buf, size_t len, loff_t* off)
{
 
    return -EINVAL;
}

long key_drv_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
 
    return 0;
}

const struct file_operations key_fops = {
    .owner   = THIS_MODULE,
    .open    =  key_drv_open,
    .release =  key_drv_release,
    .read    =  key_drv_read,
    .poll    =  key_drv_poll,
    .fasync  =  key_drv_fasync,
    .write   =  key_drv_write,
    .unlocked_ioctl = key_drv_ioctl,
};

/* MISC 设备结构体 */
static struct miscdevice key_miscdev = {
    .minor = 145,        //子设备号
    .name = "key_drv",   //注册名称
    .fops = &key_fops,
};

static irqreturn_t gpio_key_isr(int irq, void *dev_id)
{
	key_gpio_t *gpio_key = dev_id;
    mod_timer(&gpio_key->key_timer, jiffies + HZ/10);    
	return IRQ_RETVAL(IRQ_HANDLED);
}

static void key_timer_expire(struct timer_list *t)
{
	/* data ==> gpio */
    key_gpio_t *gpio_key = from_timer(gpio_key, t, key_timer);

	int val;
	val = gpiod_get_value(gpio_key->key_gpiod);

    printk("key %d %d\n", gpio_key->key_num, val);	
    g_key = (gpio_key->key_num << 8) | val;
    wake_up_interruptible(&gpio_key_wait);
    kill_fasync(&button_fasync, SIGIO, POLL_IN);  
}

static int key_probe(struct platform_device *dev)
{	
    int ret;
    
	//读节点信息
	key_dev.key_gpio[0].key_gpiod = gpiod_get(&dev->dev, "key1", 0);
    key_dev.key_gpio[1].key_gpiod = gpiod_get(&dev->dev, "key2", 0);
    key_dev.key_gpio[2].key_gpiod = gpiod_get(&dev->dev, "key3", 0);
	
    key_dev.key_gpio[0].key_num = 1;
    key_dev.key_gpio[1].key_num = 2;
    key_dev.key_gpio[2].key_num = 3;

	/* 初始化key相关 */
    ret = gpiod_direction_input(key_dev.key_gpio[0].key_gpiod);
    ret = gpiod_direction_input(key_dev.key_gpio[1].key_gpiod);
    ret = gpiod_direction_input(key_dev.key_gpio[2].key_gpiod);
 
    key_dev.key_gpio[0].key_irq = gpiod_to_irq(key_dev.key_gpio[0].key_gpiod);
    key_dev.key_gpio[1].key_irq = gpiod_to_irq(key_dev.key_gpio[1].key_gpiod);
    key_dev.key_gpio[2].key_irq = gpiod_to_irq(key_dev.key_gpio[2].key_gpiod);

	//注册gpio中断
    ret = request_irq(key_dev.key_gpio[0].key_irq, gpio_key_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "shb_gpio_key_0", &key_dev.key_gpio[0]);
    ret = request_irq(key_dev.key_gpio[1].key_irq, gpio_key_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "shb_gpio_key_1", &key_dev.key_gpio[1]);
    ret = request_irq(key_dev.key_gpio[2].key_irq, gpio_key_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "shb_gpio_key_2", &key_dev.key_gpio[2]);

    for(int i=0 ; i < 3; i++){
        timer_setup(&key_dev.key_gpio[i].key_timer, key_timer_expire, 0);
        key_dev.key_gpio[i].key_timer.expires = ~0;
        add_timer(&key_dev.key_gpio[i].key_timer);
    }
    
	/* 注册misc字符设备驱动 */
    ret = misc_register(&key_miscdev);
    if(ret < 0){
        printk("misc device register faikey!\r\n");
        return -EFAULT;
    }

	return 0;
}

static int key_remove(struct platform_device *dev)
{
    gpiod_put(key_dev.key_gpio[0].key_gpiod);
    gpiod_put(key_dev.key_gpio[1].key_gpiod);
    gpiod_put(key_dev.key_gpio[2].key_gpiod);

    for (int i = 0; i < 3; i++)
	{
		free_irq(key_dev.key_gpio[i].key_irq, &key_dev.key_gpio[i]);
		del_timer(&key_dev.key_gpio[i].key_timer);
	}

    /* 注销misc设备 */
    misc_deregister(&key_miscdev);
	return 0;
}

/* 匹配列表 */
static const struct of_device_id key_of_match[] = {
	{ .compatible = "shb-key" },
	{ /* Sentinel */ }
};

/* platform驱动结构体 */
static struct platform_driver key_driver = {
	.driver		= {
		.name	= "shb_key",			/* 驱动名字，用于和设备匹配 */
        .of_match_table = key_of_match
	},
	.probe		= key_probe,
	.remove		= key_remove,
};
		
static int __init keydriver_init(void)
{
	return platform_driver_register(&key_driver);
}

static void __exit keydriver_exit(void)
{
	platform_driver_unregister(&key_driver);
}

module_init(keydriver_init);
module_exit(keydriver_exit);

MODULE_LICENSE("GPL v2");   // 开源许可证
MODULE_AUTHOR("liqinghua"); // 模块作者