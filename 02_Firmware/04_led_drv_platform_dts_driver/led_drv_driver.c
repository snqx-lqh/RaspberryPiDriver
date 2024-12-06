/*
 * 这个驱动文件测试，树莓派使用dts和platform_driver的方式进行驱动设备注册以及使用
 * 和platform_devie的区别就是，没有直接把寄存器实际相关信息放在device文件里面了，而是通过driver去匹配设备树中的设备寄存器信息
 * 使用树莓派寄存器控制对应的引脚电平  
 * 这个工程使用的是从DTS获取寄存器相关信息
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
#include <linux/of_address.h>

/* 寄存器名 */
static void __iomem *BCM2837_GPIO_FSEL0;
static void __iomem *BCM2837_GPIO_FSEL1;
static void __iomem *BCM2837_GPIO_FSEL2;
static void __iomem *BCM2837_GPIO_SET0;
static void __iomem *BCM2837_GPIO_CLR0;
static void __iomem *BCM2837_GPIO_LEV0;

#define LED2 17
#define LED3 27
#define LED4 22
#define LED5 23

#define LED_OUTPUT 1
#define LED_INPUT  0

#define MIN(a, b) (a < b ? a : b)

typedef struct{
    dev_t devid; /* 设备号 */
    struct cdev cdev; /* cdev */
    struct device_node *node; /* LED 设备节点 */
    struct class  *class;    //类 
    struct device *device;    //设备
}led_dev_t;

static led_dev_t led_dev;

/**
 * @brief 设置对应引脚的高低电平
 * @param pin    需要设置的引脚
 * @param level  1是高电平 0是低电平
 */
void gpio_set_level(int pin, int level)
{
    void* reg = (level ? BCM2837_GPIO_SET0 : BCM2837_GPIO_CLR0);
    iowrite32(1 << pin, reg);
}

/**
 * @brief 获得引脚的电平
 * @param pin 需要获得的引脚
 * @return 1是高电平 0是低电平
 */
static int gpio_get_level(int pin)
{
    int ret = 0;
    int pin_level = 0;
    void* reg = BCM2837_GPIO_LEV0;
    // 读取引脚电平寄存器
    pin_level = ioread32( reg );
    // 将想要读取的引脚的状态，提取出来
    ret = (1 << pin );
    ret = ret & pin_level;
    if(ret){
        return 1;
    }else{
        return 0;
    }
}

/**
 * @brief 设置GPIO的寄存器，配置GPIO是输入还是输出
 * @param pin   需要设置的引脚
 * @param mode  1是输出模式 0是输入模式
 */
static void gpio_set_mode(int pin, int mode)
{
    void *reg = NULL;
    int   val = 0;
    int   pin_ctl = 0;
    // 通过pin号来确定要控制的寄存器
    if(pin < 10){
        reg = BCM2837_GPIO_FSEL0;
    }else if(pin < 20){
        reg = BCM2837_GPIO_FSEL1;
    }else if(pin < 30){
        reg = BCM2837_GPIO_FSEL2;
    }
    // 比如我要控制11号脚，就是要控制BCM2837_GPIO_FSEL1的1号位置的3个位。
    pin_ctl = pin % 10;
    // 将对应的gpio的功能选择位全部写0
    val =  ~(7   << (pin_ctl * 3));
    val &= ioread32(reg);
    // 控制设置对应的gpio的功能选择位写 000 还是 001
    val |= (mode << (pin_ctl * 3));
    iowrite32(val, reg);
}

// 通过文件读取，得到当前LED的状态
ssize_t led_drv_read(struct file* filp, char __user* buf, size_t len, loff_t* off)
{
    int ret = 0;
    int char_len = 0;
    char led_state[4];
    
    printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);
    led_state[0] = gpio_get_level(LED2);
    led_state[1] = gpio_get_level(LED3);
    led_state[2] = gpio_get_level(LED4);
    led_state[3] = gpio_get_level(LED5);

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

    switch (led_state[0])
    {
        case 0:
            gpio_set_level(LED2, led_state[1]);
            break;  
        case 1:
            gpio_set_level(LED3, led_state[1]);
            break; 
        case 2:
            gpio_set_level(LED4, led_state[1]);
            break; 
        case 3:
            gpio_set_level(LED5, led_state[1]);
            break; 
        default:
            break;
    }
    return 0;
}

const struct file_operations led_fops = {
    .owner   = THIS_MODULE,
    .read    =  led_drv_read,
    .write   =  led_drv_write,
};

static int led_probe(struct platform_device *dev)
{	
    uint32_t regdata[14];
    int ret;

	//读节点信息 
    led_dev.node = dev->dev.of_node;                  

	//第一种 读REG然后寄存器地址映射
    ret = of_property_read_u32_array(led_dev.node, "reg", regdata, 12);
    if(ret < 0) {
		printk("reg property read failed!\r\n");
	} 

  	// BCM2837_GPIO_FSEL0   = ioremap(regdata[0],  regdata[1]);
	// BCM2837_GPIO_FSEL1   = ioremap(regdata[2],  regdata[3]);
  	// BCM2837_GPIO_FSEL2   = ioremap(regdata[4],  regdata[5]);
	// BCM2837_GPIO_SET0    = ioremap(regdata[6],  regdata[7]);
	// BCM2837_GPIO_CLR0    = ioremap(regdata[8],  regdata[9]);
    // BCM2837_GPIO_LEV0    = ioremap(regdata[10], regdata[11]);

    // 第二种 使用节点信息，寄存器地址映射
    BCM2837_GPIO_FSEL0 = of_iomap(led_dev.node,  0);
	BCM2837_GPIO_FSEL1 = of_iomap(led_dev.node,  1);
  	BCM2837_GPIO_FSEL2 = of_iomap(led_dev.node,  2);
	BCM2837_GPIO_SET0  = of_iomap(led_dev.node,  3);
	BCM2837_GPIO_CLR0  = of_iomap(led_dev.node,  4);
    BCM2837_GPIO_LEV0  = of_iomap(led_dev.node,  5);
	
	/* 初始化LED相关 */
    gpio_set_mode(LED2, LED_OUTPUT);
    gpio_set_mode(LED3, LED_OUTPUT);
    gpio_set_mode(LED4, LED_OUTPUT);
    gpio_set_mode(LED5, LED_OUTPUT);

    // 设置引脚电平
    gpio_set_level(LED2, 0);
    gpio_set_level(LED3, 0);
    gpio_set_level(LED4, 0);
    gpio_set_level(LED5, 0);

	/* 注册字符设备驱动 */
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
    printk("%s %s line %d\r\n", __FILE__, __FUNCTION__, __LINE__);
    // 设置电平为高
    gpio_set_level(LED2, 1);
    gpio_set_level(LED3, 1);
    gpio_set_level(LED4, 1);
    gpio_set_level(LED5, 1);
    // 取消gpio物理内存映射
    iounmap(BCM2837_GPIO_FSEL0);
    iounmap(BCM2837_GPIO_FSEL1);
    iounmap(BCM2837_GPIO_FSEL2);
    iounmap(BCM2837_GPIO_SET0);
    iounmap(BCM2837_GPIO_CLR0);
    iounmap(BCM2837_GPIO_LEV0);

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