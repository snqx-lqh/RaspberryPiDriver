/*
 * 这个驱动文件测试，树莓派使用dts和platform_driver的方式进行驱动设备注册以及使用
 * 和上一个示例的区别就是，在设备树中使用了pinctrl，还使用了gpio子系统，就不需要写寄存器映射相关的内容了
 * gpio子系统有传统gpio开头的api以及后面gpiod开头的api
 * 使用树莓派gpio子系统的gpiod开头的api控制对应的引脚电平  
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
#include <linux/gpio/consumer.h>

typedef struct{
    dev_t devid; /* 设备号 */
    struct cdev cdev; /* cdev */
    struct device_node *node; /* LED 设备节点 */
    struct class  *class;    //类 
    struct device *device;    //设备 
    struct gpio_desc * led_gpiod[4];
}led_dev_t;

static led_dev_t led_dev;

#define MIN(a, b) (a < b ? a : b)

// 通过文件读取，得到当前LED的状态
ssize_t led_drv_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
    int ret = 0;
    int char_len = 0;
    char led_state[4];
    
    printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);
    led_state[0] = gpiod_get_value(led_dev.led_gpiod[0]);
    led_state[1] = gpiod_get_value(led_dev.led_gpiod[1]);
    led_state[2] = gpiod_get_value(led_dev.led_gpiod[2]);
    led_state[3] = gpiod_get_value(led_dev.led_gpiod[3]);

    char_len = sizeof(led_state);
    int real_len = MIN(len,char_len);
    ret = copy_to_user(buf, led_state, real_len);
    return ret < 0 ? ret : real_len;
}

// 通过向文件写入LED状态，控制LED灯
ssize_t led_drv_write(struct file* filp, const char __user* buf, size_t len, loff_t* off)
{
    int ret = 0;
    char led_state[4];
    int char_len = 0;

    printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);
    char_len = sizeof(led_state);
    int real_len = MIN(len,char_len);
    ret = copy_from_user(led_state, buf, real_len);

    if(led_state[0] >= 0 && led_state[0] <= 3)
        gpiod_set_value(led_dev.led_gpiod[(int)led_state[0]], led_state[1]);
    else
        printk("can't control %d", led_state[0]);
    return -EINVAL;
}

const struct file_operations led_fops = {
    .owner   = THIS_MODULE,
    .read    =  led_drv_read,
    .write   =  led_drv_write,
};

static int led_probe(struct platform_device *dev)
{	
    int ret;
    
    // 获取GPIO描述符
    led_dev.led_gpiod[0] = gpiod_get(&dev->dev,"led1",0);
    led_dev.led_gpiod[1] = gpiod_get(&dev->dev,"led2",0);
    led_dev.led_gpiod[2] = gpiod_get(&dev->dev,"led3",0);
    led_dev.led_gpiod[3] = gpiod_get(&dev->dev,"led4",0);

	//设置gpio为输出模式，同时初始化为低电平
    ret = gpiod_direction_output(led_dev.led_gpiod[0], 0);
    ret = gpiod_direction_output(led_dev.led_gpiod[1], 0);
    ret = gpiod_direction_output(led_dev.led_gpiod[2], 0);
    ret = gpiod_direction_output(led_dev.led_gpiod[3], 0);

    // 将该模块注册为一个字符设备，并动态分配设备号
    if (alloc_chrdev_region(&led_dev.devid, 0, 1, "led_drv")) {
        printk(KERN_ERR"failed to register kernel module!\n");
        return -1;
    }
    cdev_init(&led_dev.cdev, &led_fops);
    cdev_add(&led_dev.cdev, led_dev.devid, 1);

    //创建类 
    led_dev.class = class_create("led_drv");
    if (IS_ERR(led_dev.class)) {
        return PTR_ERR(led_dev.class);
    }
    
    //创建设备
    led_dev.device = device_create(led_dev.class, NULL, led_dev.devid, NULL, "led_drv");
    if (IS_ERR(led_dev.device)) {
        return PTR_ERR(led_dev.device);
    }
	return 0;
}

static int led_remove(struct platform_device *dev)
{
    //设置引脚电平为高电平
    gpiod_set_value(led_dev.led_gpiod[0], 1);
    gpiod_set_value(led_dev.led_gpiod[1], 1);
    gpiod_set_value(led_dev.led_gpiod[2], 1);
    gpiod_set_value(led_dev.led_gpiod[3], 1);
    //释放引脚
    gpiod_put(led_dev.led_gpiod[0]);
    gpiod_put(led_dev.led_gpiod[1]);
    gpiod_put(led_dev.led_gpiod[2]);
    gpiod_put(led_dev.led_gpiod[3]);

    cdev_del(&led_dev.cdev);
    unregister_chrdev_region(led_dev.devid, 1);

    //删除类和设备
    device_destroy(led_dev.class, led_dev.devid);
    class_destroy(led_dev.class);
	return 0;
}

/* 匹配列表 */
static const struct of_device_id led_of_match[] = {
	{ .compatible = "shb-led" },
	{ /* Sentinel */ }
};

/* platform驱动结构体 */
static struct platform_driver led_driver = {
	.driver		= {
		.name	= "shbled",			/* 驱动名字，用于和设备匹配 */
        .of_match_table = led_of_match
	},
	.probe		= led_probe,
	.remove		= led_remove,
};
		
static int __init leddriver_init(void)
{
	return platform_driver_register(&led_driver);
}

static void __exit leddriver_exit(void)
{
	platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);

MODULE_LICENSE("GPL v2");   // 开源许可证
MODULE_AUTHOR("liqinghua"); // 模块作者